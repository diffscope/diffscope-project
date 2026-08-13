#include "SynthesisTaskCache.h"

#include <utility>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

#include <CoreApi/runtimeinterface.h>

#include <synth/internal/SynthesisTaskCodec.h>

namespace Synth::Internal {

    using namespace TaskCodec;

    SynthesisTaskCache::SynthesisTaskCache() {
        reload();
    }

    void SynthesisTaskCache::reload() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.synth"));
        m_maximumBytes = settings->value(QStringLiteral("cacheMaximumBytes"), qint64(10) * 1024 * 1024 * 1024).toLongLong();
        m_expiryDays = settings->value(QStringLiteral("cacheExpiryDays"), 30).toInt();
        m_maximumDownloadBytes = settings->value(QStringLiteral("audioDownloadMaximumBytes"), qint64(512) * 1024 * 1024).toLongLong();
        m_environmentTagTtlSeconds = settings->value(QStringLiteral("environmentTagTtlSeconds"), 60).toInt();
        settings->endGroup();
        QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        if (cacheLocation.isEmpty()) {
            cacheLocation = QDir(QDir::tempPath()).filePath(QStringLiteral("DiffScope-cache"));
        }
        m_root = QDir(cacheLocation).filePath(QStringLiteral("synth/v1"));
        QDir().mkpath(m_root);
        trim();
    }

    bool SynthesisTaskCache::read(const QByteArray &key, SynthesisTaskResult *result) const {
        const QFileInfo info(entryPath(key));
        if (!info.exists()) {
            return false;
        }
        if (m_expiryDays > 0 && info.lastModified().daysTo(QDateTime::currentDateTime()) > m_expiryDays) {
            return false;
        }
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            return false;
        }
        SynthesisTaskResult parsed;
        if (!resultFromJson(document.object().value(QStringLiteral("result")).toObject(), &parsed)) {
            return false;
        }
        if (!parsed.audioFilePath.isEmpty() && !QFileInfo::exists(parsed.audioFilePath)) {
            return false;
        }
        file.setFileTime(QDateTime::currentDateTime(), QFileDevice::FileModificationTime);
        if (!parsed.audioFilePath.isEmpty()) {
            QFile audioFile(parsed.audioFilePath);
            if (audioFile.open(QIODevice::ReadOnly)) {
                audioFile.setFileTime(QDateTime::currentDateTime(), QFileDevice::FileModificationTime);
            }
        }
        parsed.fromCache = true;
        *result = std::move(parsed);
        return true;
    }

    bool SynthesisTaskCache::write(const QByteArray &key, const SynthesisTaskResult &result) {
        QSaveFile file(entryPath(key));
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        file.write(QJsonDocument(QJsonObject{
                                     {QStringLiteral("version"), 1},
                                     {QStringLiteral("result"), resultToJson(result)},
                                 })
                       .toJson(QJsonDocument::Compact));
        const bool committed = file.commit();
        if (committed) {
            trim();
        }
        return committed;
    }

    QString SynthesisTaskCache::audioPath(const QByteArray &key, const QString &suffix, bool persistent) const {
        const QString safeSuffix = suffix.isEmpty() ? QStringLiteral(".audio") : suffix;
        if (persistent) {
            return QDir(m_root).filePath(QString::fromLatin1(key) + safeSuffix);
        }
        if (!m_temporaryDir.isValid()) {
            return {};
        }
        return QDir(m_temporaryDir.path()).filePath(QString::fromLatin1(key) + safeSuffix);
    }

    bool SynthesisTaskCache::writeBytes(const QString &path, const QByteArray &bytes) const {
        if (path.isEmpty()) {
            return false;
        }
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()) {
            return false;
        }
        return file.commit();
    }

    qint64 SynthesisTaskCache::size() const {
        qint64 total{};
        const auto files = QDir(m_root).entryInfoList(QDir::Files);
        for (const auto &file : files) {
            total += file.size();
        }
        return total;
    }

    void SynthesisTaskCache::clear() {
        const auto files = QDir(m_root).entryInfoList(QDir::Files);
        for (const auto &file : files) {
            QFile::remove(file.absoluteFilePath());
        }
    }

    void SynthesisTaskCache::trim() const {
        auto files = QDir(m_root).entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);
        qint64 total{};
        const auto now = QDateTime::currentDateTime();
        for (const auto &file : files) {
            if (m_expiryDays > 0 && file.lastModified().daysTo(now) > m_expiryDays) {
                QFile::remove(file.absoluteFilePath());
            } else {
                total += file.size();
            }
        }
        if (m_maximumBytes <= 0 || total <= m_maximumBytes) {
            return;
        }
        files = QDir(m_root).entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);
        for (const auto &file : files) {
            if (total <= m_maximumBytes) {
                break;
            }
            const auto bytes = file.size();
            if (QFile::remove(file.absoluteFilePath())) {
                total -= bytes;
            }
        }
    }

    qint64 SynthesisTaskCache::maximumDownloadBytes() const {
        return m_maximumDownloadBytes;
    }

    int SynthesisTaskCache::environmentTagTtlSeconds() const {
        return m_environmentTagTtlSeconds;
    }

    QString SynthesisTaskCache::entryPath(const QByteArray &key) const {
        return QDir(m_root).filePath(QString::fromLatin1(key) + QStringLiteral(".json"));
    }

}
