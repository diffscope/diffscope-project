// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DownloadCatalog_p.h"

#include <cmath>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSet>
#include <QSysInfo>

#include <SVSCraftCore/Semver.h>

namespace AppInstall {

    namespace {

        const QStringList channelIds{QStringLiteral("stable"), QStringLiteral("beta"), QStringLiteral("nightly")};

        bool isHttpUrl(const QUrl &url) {
            return url.isValid() && !url.host().isEmpty() && (url.scheme() == QStringLiteral("https") || url.scheme() == QStringLiteral("http"));
        }

        QString architecture(QString value) {
            value = value.toLower();
            if (value == QStringLiteral("x86_64") || value == QStringLiteral("x64"))
                return QStringLiteral("amd64");
            if (value == QStringLiteral("aarch64"))
                return QStringLiteral("arm64");
            if (value == QStringLiteral("i386") || value == QStringLiteral("i686") || value == QStringLiteral("x86"))
                return QStringLiteral("386");
            return value;
        }

        bool matchesSystem(const QJsonObject &artifact, const QString &variant) {
#if defined(Q_OS_WIN)
            const auto platform = QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
            const auto platform = QStringLiteral("macos");
#elif defined(Q_OS_LINUX)
            const auto platform = QStringLiteral("linux");
#else
            return false;
#endif
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
            const auto arch = architecture(artifact.value(QStringLiteral("arch")).toString());
            return artifact.value(QStringLiteral("platform")).toString() == platform && artifact.value(QStringLiteral("variant")).toString() == variant &&
                   (arch == architecture(QSysInfo::currentCpuArchitecture()) || arch == QStringLiteral("universal") || arch == QStringLiteral("universal2") || arch == QStringLiteral("any"));
#endif
        }

    }

    DownloadCatalog::DownloadCatalog(const QUrl &catalogUrl, const QString &variant, QObject *parent) : QObject(parent), m_catalogUrl(catalogUrl), m_variant(variant), m_session(QUrl{}, &m_data) {
        m_session.setUserAgent(QStringLiteral("DiffScope/%1").arg(QCoreApplication::applicationVersion()));
        m_timeout.setSingleShot(true);
        m_timeout.setInterval(15000);
        connect(&m_timeout, &QTimer::timeout, this, [this] {
            m_errorMessage = tr("Retrieving the download catalog timed out.");
            m_session.cancel();
        });
        connect(&m_session, &SVS::DownloadSession::networkErrorOccurred, this, [this](QNetworkReply::NetworkError code, const QString &description) {
            m_notFound = code == QNetworkReply::ContentNotFoundError;
            m_errorMessage = description;
        });
        connect(&m_session, &SVS::DownloadSession::writeErrorOccurred, this, [this](const QString &description) { m_errorMessage = description; });
        connect(&m_session, &SVS::DownloadSession::finished, this, &DownloadCatalog::completeRequest, Qt::QueuedConnection);
    }

    QVariantList DownloadCatalog::channels() const {
        const QStringList names{tr("Stable"), tr("Beta"), tr("Nightly")};
        QVariantList result;
        for (int i = 0; i < 3; ++i)
            result.append(QVariantMap{{QStringLiteral("name"), names.at(i)}, {QStringLiteral("available"), !m_releases.at(i).isEmpty()}});
        return result;
    }

    QStringList DownloadCatalog::versions() const {
        QStringList result;
        for (const auto &release : m_releases.at(m_channelIndex))
            result.append(release.version);
        return result;
    }

    int DownloadCatalog::channelIndex() const {
        return m_channelIndex;
    }

    int DownloadCatalog::versionIndex() const {
        return m_versionIndex;
    }

    bool DownloadCatalog::busy() const {
        return m_busy;
    }

    bool DownloadCatalog::available() const {
        return !m_busy && m_errorMessage.isEmpty() && !artifact().url.isEmpty();
    }

    QString DownloadCatalog::errorMessage() const {
        return m_errorMessage;
    }

    QUrl DownloadCatalog::releaseNotes() const {
        return artifact().releaseNotes;
    }

    DownloadArtifact DownloadCatalog::artifact() const {
        return m_artifacts.value(selectionKey(m_channelIndex, m_versionIndex));
    }

    QVariantMap DownloadCatalog::latestVersions() const {
        QVariantMap result;
        for (int i = 0; i < 3; ++i) {
            const auto artifact = m_artifacts.value(selectionKey(i, 0));
            if (!artifact.url.isEmpty())
                result.insert(channelIds.at(i), artifact.version);
        }
        return result;
    }

    void DownloadCatalog::setChannelIndex(int index) {
        if (m_busy || index < 0 || index >= 3 || index == m_channelIndex || m_releases.at(index).isEmpty())
            return;
        m_channelIndex = index;
        m_versionIndex = 0;
        loadSelection();
    }

    void DownloadCatalog::setVersionIndex(int index) {
        if (m_busy || index < 0 || index >= m_releases.at(m_channelIndex).size() || index == m_versionIndex)
            return;
        m_versionIndex = index;
        loadSelection();
    }

    void DownloadCatalog::start() {
        if (m_busy)
            return;
        m_initialized = false;
        m_catalogFound = false;
        m_releases = {};
        m_artifacts.clear();
        m_channelIndex = 0;
        m_versionIndex = 0;
        request(m_catalogUrl.resolved(QUrl(QStringLiteral("index.json"))), Request::Index);
    }

    void DownloadCatalog::cancel() {
        m_active = false;
        m_timeout.stop();
        m_session.cancel();
        m_busy = false;
    }

    void DownloadCatalog::retry() {
        if (m_busy)
            return;
        if (m_initialized)
            loadSelection();
        else
            start();
    }

    void DownloadCatalog::request(const QUrl &url, Request kind) {
        m_request = kind;
        m_busy = true;
        m_active = true;
        m_notFound = false;
        m_errorMessage.clear();
        m_session.reset();
        m_data.close();
        m_data.setData({});
        if (!isHttpUrl(url) || !m_data.open(QIODevice::WriteOnly)) {
            fail(tr("The download catalog URL is invalid."));
            return;
        }
        m_session.setUrl(url);
        m_timeout.start();
        Q_EMIT changed();
        m_session.start();
    }

    void DownloadCatalog::completeRequest() {
        if (!m_active)
            return;
        m_active = false;
        m_timeout.stop();
        const bool optionalMissing = m_notFound && (m_request == Request::Index || m_request == Request::Nightly);
        if (!optionalMissing && (!m_errorMessage.isEmpty() || m_session.status() != SVS::DownloadSession::Status::Completed)) {
            fail(m_errorMessage.isEmpty() ? tr("The download catalog could not be retrieved.") : m_errorMessage);
            return;
        }
        if (m_request == Request::Index || m_request == Request::Nightly)
            m_catalogFound |= !optionalMissing;
        if (m_request == Request::Index) {
            if (!optionalMissing && (!readIndex(0) || !readIndex(1)))
                return;
            request(m_catalogUrl.resolved(QUrl(QStringLiteral("nightly.json"))), Request::Nightly);
        } else if (m_request == Request::Nightly) {
            if (!m_catalogFound) {
                fail(m_errorMessage.isEmpty() ? tr("The download catalog could not be retrieved.") : m_errorMessage);
                return;
            }
            if (!optionalMissing && !readIndex(2))
                return;
            m_preferredIndex = 0;
            findPreferredChannel();
        } else if (m_request == Request::Preferred) {
            if (!readManifest(m_preferredIndex, 0))
                return;
            ++m_preferredIndex;
            findPreferredChannel();
        } else {
            if (!readManifest(m_channelIndex, m_versionIndex))
                return;
            m_busy = false;
            Q_EMIT changed();
        }
    }

    bool DownloadCatalog::readIndex(int channel) {
        const auto object = QJsonDocument::fromJson(m_data.data()).object();
        const auto versions = object.value(channelIds.at(channel)).toObject().value(QStringLiteral("versions"));
        if (!versions.isArray()) {
            fail(tr("The download catalog is invalid."));
            return false;
        }
        QSet<QString> seen;
        for (const auto &value : versions.toArray()) {
            const auto entry = value.toObject();
            const auto version = entry.value(QStringLiteral("version")).toString();
            const auto manifest = entry.value(QStringLiteral("manifest")).toString();
            const auto url = m_session.url().resolved(QUrl(manifest));
            if (!SVS::Semver(version).isValid() || !QRegularExpression(QStringLiteral("^[A-Za-z0-9.+-]+$")).match(version).hasMatch() || seen.contains(version) || manifest.isEmpty() || !isHttpUrl(url)) {
                fail(tr("The download catalog is invalid."));
                return false;
            }
            seen.insert(version);
            m_releases.at(channel).append({version, url});
        }
        return true;
    }

    bool DownloadCatalog::readManifest(int channel, int version) {
        const auto object = QJsonDocument::fromJson(m_data.data()).object();
        const auto &release = m_releases.at(channel).at(version);
        const auto values = object.value(QStringLiteral("artifacts"));
        if (object.value(QStringLiteral("channel")).toString() != channelIds.at(channel) || object.value(QStringLiteral("version")).toString() != release.version || !values.isArray()) {
            fail(tr("The release manifest is invalid."));
            return false;
        }
        DownloadArtifact selected;
        selected.channel = channelIds.at(channel);
        selected.version = release.version;
        const auto notes = object.value(QStringLiteral("releaseNotes")).toString();
        if (!notes.isEmpty() && isHttpUrl(QUrl(notes)))
            selected.releaseNotes = QUrl(notes);
        for (const auto &value : values.toArray()) {
            const auto artifact = value.toObject();
            if (!matchesSystem(artifact, m_variant))
                continue;
            const auto url = QUrl(artifact.value(QStringLiteral("url")).toString());
            const auto size = artifact.value(QStringLiteral("size")).toDouble(-1);
            const auto sha256 = artifact.value(QStringLiteral("sha256")).toString();
            if (!isHttpUrl(url) || !std::isfinite(size) || size < 0 || size > 9007199254740991.0 || std::floor(size) != size || !QRegularExpression(QStringLiteral("^[a-fA-F0-9]{64}$")).match(sha256).hasMatch()) {
                fail(tr("The release manifest is invalid."));
                return false;
            }
            selected.url = url;
            selected.size = static_cast<qint64>(size);
            selected.sha256 = QByteArray::fromHex(sha256.toLatin1());
            break;
        }
        m_artifacts.insert(selectionKey(channel, version), selected);
        return true;
    }

    void DownloadCatalog::findPreferredChannel() {
        while (m_preferredIndex < 3 && m_releases.at(m_preferredIndex).isEmpty())
            ++m_preferredIndex;
        if (m_preferredIndex < 3) {
            request(m_releases.at(m_preferredIndex).first().manifest, Request::Preferred);
            return;
        }
        m_channelIndex = 0;
        for (int i = 0; i < 3; ++i) {
            if (!m_releases.at(i).isEmpty()) {
                m_channelIndex = i;
                break;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!m_artifacts.value(selectionKey(i, 0)).url.isEmpty()) {
                m_channelIndex = i;
                break;
            }
        }
        m_initialized = true;
        m_busy = false;
        m_errorMessage.clear();
        Q_EMIT changed();
    }

    void DownloadCatalog::loadSelection() {
        m_errorMessage.clear();
        if (m_releases.at(m_channelIndex).isEmpty()) {
            Q_EMIT changed();
            return;
        }
        if (m_artifacts.contains(selectionKey(m_channelIndex, m_versionIndex))) {
            Q_EMIT changed();
            return;
        }
        request(m_releases.at(m_channelIndex).at(m_versionIndex).manifest, Request::Selection);
    }

    void DownloadCatalog::fail(const QString &message) {
        m_active = false;
        m_busy = false;
        m_timeout.stop();
        m_errorMessage = message;
        Q_EMIT changed();
    }

    QString DownloadCatalog::selectionKey(int channel, int version) const {
        return QString::number(channel) + u':' + QString::number(version);
    }

}

#include "moc_DownloadCatalog_p.cpp"
