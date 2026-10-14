// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#include "LibreSVIPDownloadCatalog.h"

#include <algorithm>

#include <QByteArray>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSysInfo>
#include <QTimer>

namespace LibreSVIPFormatConverter::Internal {

    static bool isDownloadUrl(const QUrl &url) {
        return url.isValid() && !url.host().isEmpty() &&
               (url.scheme() == QStringLiteral("https") || url.scheme() == QStringLiteral("http"));
    }

    static bool matchesCurrentSystem(const QJsonObject &artifact) {
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
#if defined(Q_OS_WIN)
        const QString platform = QStringLiteral("windows");
        const QString format = QStringLiteral("zip");
#elif defined(Q_OS_MACOS)
        const QString platform = QStringLiteral("macos");
        const QString format = QStringLiteral("tar.gz");
#elif defined(Q_OS_LINUX)
        const QString platform = QStringLiteral("linux");
        const QString format = QStringLiteral("tar.gz");
#endif

        QString architecture = QSysInfo::currentCpuArchitecture().toLower();
        if (architecture == QStringLiteral("x86_64") || architecture == QStringLiteral("x64"))
            architecture = QStringLiteral("amd64");
        else if (architecture == QStringLiteral("aarch64"))
            architecture = QStringLiteral("arm64");
        else if (architecture == QStringLiteral("i386") || architecture == QStringLiteral("i686"))
            architecture = QStringLiteral("x86");
        return artifact.value(QStringLiteral("platform")).toString() == platform &&
               artifact.value(QStringLiteral("arch")).toString() == architecture &&
               artifact.value(QStringLiteral("format")).toString() == format &&
               artifact.value(QStringLiteral("variant")).toString() == QStringLiteral("portable");
#else
        Q_UNUSED(artifact);
        return false;
#endif
    }

    LibreSVIPDownloadCatalog::LibreSVIPDownloadCatalog(QObject *parent)
        : QObject(parent), m_session(QUrl(), &m_data) {
        m_session.setUserAgent(QStringLiteral("DiffScope/%1").arg(QCoreApplication::applicationVersion()));
        connect(&m_session, &SVS::DownloadSession::progressChanged, this, &LibreSVIPDownloadCatalog::progressChanged);
        connect(&m_session, &SVS::DownloadSession::networkErrorOccurred, this,
                [this](QNetworkReply::NetworkError, const QString &description) { m_errorMessage = description; });
        connect(&m_session, &SVS::DownloadSession::writeErrorOccurred, this,
                [this](const QString &description) { m_errorMessage = description; });
        // Process completion after error details are recorded and the network reply is released.
        connect(&m_session, &SVS::DownloadSession::finished, this, [this] {
            if (!m_active)
                return;
            if (!m_errorMessage.isEmpty() || m_session.status() != SVS::DownloadSession::Status::Completed) {
                if (m_errorMessage.isEmpty()) {
                    m_errorMessage = QCoreApplication::translate(
                        "LibreSVIPFormatConverter::Internal::LibreSVIPManager", "The LibreSVIP download catalog could not be retrieved.");
                }
                finish();
                return;
            }
            processDocument();
        }, Qt::QueuedConnection);
    }

    const QList<LibreSVIPDownloadArtifact> &LibreSVIPDownloadCatalog::artifacts() const {
        return m_artifacts;
    }

    QString LibreSVIPDownloadCatalog::errorString() const {
        return m_errorMessage;
    }

    void LibreSVIPDownloadCatalog::start() {
        if (m_active)
            return;
        m_active = true;
        m_releaseIndex = -1;
        m_releases.clear();
        m_artifacts.clear();
        m_errorMessage.clear();
        request(QUrl(QString::fromUtf8(DIFFSCOPE_LIBRESVIP_CATALOG_URL)));
    }

    void LibreSVIPDownloadCatalog::cancel() {
        m_active = false;
        m_session.cancel();
    }

    void LibreSVIPDownloadCatalog::request(const QUrl &url) {
        m_session.reset();
        m_data.close();
        m_data.setData(QByteArray());
        if (!isDownloadUrl(url) || !m_data.open(QIODevice::WriteOnly)) {
            m_errorMessage = QCoreApplication::translate(
                "LibreSVIPFormatConverter::Internal::LibreSVIPManager", "The LibreSVIP download catalog could not be retrieved.");
            finish();
            return;
        }
        m_session.setUrl(url);
        m_session.start();
    }

    void LibreSVIPDownloadCatalog::processDocument() {
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(m_data.data(), &parseError);
        const auto invalidCatalog = [this](const QString &detail) {
            m_errorMessage = QCoreApplication::translate(
                                 "LibreSVIPManager", "The LibreSVIP download catalog is invalid: %1")
                                 .arg(detail);
            finish();
        };
        if (parseError.error != QJsonParseError::NoError) {
            invalidCatalog(parseError.errorString());
            return;
        }
        if (!document.isObject()) {
            invalidCatalog(QCoreApplication::translate("LibreSVIPManager", "The root value is not an object."));
            return;
        }
        const auto object = document.object();
        if (m_releaseIndex < 0) {
            const auto versions = object.value(QStringLiteral("stable")).toObject().value(QStringLiteral("versions"));
            if (!versions.isArray()) {
                invalidCatalog(m_session.url().toString());
                return;
            }
            for (const auto &value : versions.toArray()) {
                const auto entry = value.toObject();
                const QString versionText = entry.value(QStringLiteral("version")).toString();
                const SVS::Semver version(versionText);
                const QString manifest = entry.value(QStringLiteral("manifest")).toString();
                const QUrl manifestUrl = m_session.url().resolved(QUrl(manifest));
                if (!version.isValid() || manifest.isEmpty() || !isDownloadUrl(manifestUrl)) {
                    invalidCatalog(m_session.url().toString());
                    return;
                }
                m_releases.append({version, versionText, manifestUrl});
            }
            std::sort(m_releases.begin(), m_releases.end(), [](const auto &left, const auto &right) {
                return left.version > right.version;
            });
        } else {
            const auto &release = m_releases.at(m_releaseIndex);
            const auto artifacts = object.value(QStringLiteral("artifacts"));
            if (object.value(QStringLiteral("channel")).toString() != QStringLiteral("stable") ||
                object.value(QStringLiteral("version")).toString() != release.versionText || !artifacts.isArray()) {
                invalidCatalog(release.manifestUrl.toString());
                return;
            }
            for (const auto &value : artifacts.toArray()) {
                const auto artifact = value.toObject();
                if (!matchesCurrentSystem(artifact))
                    continue;
                const QUrl url(artifact.value(QStringLiteral("url")).toString());
                if (!isDownloadUrl(url)) {
                    invalidCatalog(release.manifestUrl.toString());
                    return;
                }
                m_artifacts.append({release.version, release.versionText, url});
                break;
            }
        }
        ++m_releaseIndex;
        if (m_releaseIndex == m_releases.size()) {
            if (m_artifacts.isEmpty()) {
                m_errorMessage = QCoreApplication::translate(
                    "LibreSVIPManager", "No LibreSVIP downloads are available for this system architecture.");
            }
            finish();
            return;
        }
        QTimer::singleShot(0, this, [this] {
            if (m_active)
                request(m_releases.at(m_releaseIndex).manifestUrl);
        });
    }

    void LibreSVIPDownloadCatalog::finish() {
        m_active = false;
        Q_EMIT finished();
    }

}
