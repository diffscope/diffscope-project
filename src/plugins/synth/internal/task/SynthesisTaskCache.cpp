#include "SynthesisTaskCache.h"

#include <algorithm>
#include <utility>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/runtimeinterface.h>

#include <synth/internal/SynthesisTaskCodec.h>

namespace Synth::Internal {

    using namespace TaskCodec;

    namespace {

        const QList<SynthesisTaskType> &cacheTaskTypes() {
            static const QList types{
                SynthesisTaskType::Pronunciation,
                SynthesisTaskType::Phoneme,
                SynthesisTaskType::Duration,
                SynthesisTaskType::Parameter,
                SynthesisTaskType::Audio,
            };
            return types;
        }

    }

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
        const auto runtimeData = Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::RuntimeData);
        m_root = QDir(runtimeData).filePath(QStringLiteral("synth/cache-v1"));
        for (const auto type : cacheTaskTypes()) {
            QDir().mkpath(typeDirectory(type));
        }
        trim();
    }

    bool SynthesisTaskCache::read(SynthesisTaskType type, const QByteArray &key, SynthesisTaskResult *result) const {
        const QFileInfo info(entryPath(type, key));
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
        const auto object = document.object();
        if (object.value(QStringLiteral("version")).toInt() != 1 ||
            object.value(QStringLiteral("taskType")).toString() != typeName(type) ||
            object.value(QStringLiteral("key")).toString().toLatin1() != key) {
            return false;
        }
        SynthesisTaskResult parsed;
        if (!resultFromJson(object.value(QStringLiteral("result")).toObject(), &parsed)) {
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

    bool SynthesisTaskCache::write(SynthesisTaskType type, const QByteArray &key, const SynthesisTaskResult &result) {
        QSaveFile file(entryPath(type, key));
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        file.write(QJsonDocument(QJsonObject{
                                     {QStringLiteral("version"), 1},
                                     {QStringLiteral("taskType"), typeName(type)},
                                     {QStringLiteral("key"), QString::fromLatin1(key)},
                                     {QStringLiteral("result"), resultToJson(result)},
                                 })
                       .toJson(QJsonDocument::Compact));
        return file.commit();
    }

    QString SynthesisTaskCache::audioPath(SynthesisTaskType type, const QByteArray &key, const QString &suffix, bool persistent) const {
        const QString safeSuffix = suffix.isEmpty() ? QStringLiteral(".audio") : suffix;
        if (persistent) {
            return QDir(typeDirectory(type)).filePath(QString::fromLatin1(key) + safeSuffix);
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
        for (const auto type : cacheTaskTypes()) {
            total += size(type);
        }
        return total;
    }

    qint64 SynthesisTaskCache::size(SynthesisTaskType type) const {
        qint64 total{};
        const auto files = QDir(typeDirectory(type)).entryInfoList(QDir::Files);
        for (const auto &file : files) {
            total += file.size();
        }
        return total;
    }

    void SynthesisTaskCache::clear() {
        clear(cacheTaskTypes());
    }

    void SynthesisTaskCache::clear(const QList<SynthesisTaskType> &types) {
        for (const auto type : types) {
            const auto files = QDir(typeDirectory(type)).entryInfoList(QDir::Files);
            for (const auto &file : files) {
                QFile::remove(file.absoluteFilePath());
            }
        }
    }

    void SynthesisTaskCache::trim() const {
        QFileInfoList files;
        for (const auto type : cacheTaskTypes()) {
            files.append(QDir(typeDirectory(type)).entryInfoList(QDir::Files));
        }
        std::ranges::sort(files, [](const QFileInfo &left, const QFileInfo &right) {
            return left.lastModified() < right.lastModified();
        });
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

    QString SynthesisTaskCache::typeDirectory(SynthesisTaskType type) const {
        return QDir(m_root).filePath(typeName(type));
    }

    QString SynthesisTaskCache::entryPath(SynthesisTaskType type, const QByteArray &key) const {
        return QDir(typeDirectory(type)).filePath(QString::fromLatin1(key) + QStringLiteral(".json"));
    }

}
