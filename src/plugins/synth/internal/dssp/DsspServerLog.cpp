// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DsspServerLog.h"

#include <array>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef Q_OS_WIN
#include <QtZlib/zlib.h>
#else
#include <zlib.h>
#endif

#include <CoreApi/applicationinfo.h>

namespace Synth::Internal {

    namespace {

        QByteArray compressData(const QByteArray &input, int level) {
            z_stream stream{};
            if (deflateInit2(&stream, level, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
                return {};
            stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.constData()));
            stream.avail_in = static_cast<uInt>(input.size());
            QByteArray output;
            std::array<char, 16384> buffer;
            int result;
            do {
                stream.next_out = reinterpret_cast<Bytef *>(buffer.data());
                stream.avail_out = static_cast<uInt>(buffer.size());
                result = deflate(&stream, stream.avail_in ? Z_NO_FLUSH : Z_FINISH);
                if (result != Z_OK && result != Z_STREAM_END) {
                    deflateEnd(&stream);
                    return {};
                }
                output.append(buffer.data(), static_cast<qsizetype>(buffer.size() - stream.avail_out));
            } while (result != Z_STREAM_END);
            deflateEnd(&stream);
            return output;
        }

        QString generateLogFileName() {
            const QDir directory(DsspServerLog::logsLocation());
            const auto timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).replace(u':', u'-');
            for (int counter = 0;; ++counter) {
                const auto name = counter == 0 ? timestamp : timestamp + u'_' + QString::number(counter);
                const auto path = directory.filePath(name + QStringLiteral(".log"));
                // Restarts within the same second must also preserve archived sessions.
                if (!QFile::exists(path) && !QFile::exists(path + QStringLiteral(".gz")))
                    return path;
            }
        }

    }

    DsspServerLog::DsspServerLog(QObject *parent) : QObject(parent) {
    }

    void DsspServerLog::setConfiguration(const DsspLogConfiguration &configuration) {
        m_configuration = configuration;
    }

    void DsspServerLog::beginSession() {
        endSession();
        m_errorMessage.clear();
        QDir directory(logsLocation());
        if (!directory.mkpath(QStringLiteral("."))) {
            setError(tr("Could not create the log directory:\n%1").arg(logsLocation()));
            return;
        }
        archiveExistingLogFiles();
        rotateLogFile();
    }

    void DsspServerLog::append(const QByteArray &data) {
        if (data.isEmpty())
            return;
        m_pendingLine.append(data);
        qsizetype offset = 0;
        qsizetype newline;
        while ((newline = m_pendingLine.indexOf('\n', offset)) >= 0) {
            const auto line = m_pendingLine.mid(offset, newline - offset + 1);
            writeLine(line);
            processLine(line);
            offset = newline + 1;
        }
        m_pendingLine.remove(0, offset);
    }

    void DsspServerLog::endSession() {
        if (!m_pendingLine.isEmpty()) {
            writeLine(m_pendingLine);
            processLine(m_pendingLine);
            m_pendingLine.clear();
        }
        m_file.close();
    }

    QString DsspServerLog::logsLocation() {
        return QDir::toNativeSeparators(QDir::cleanPath(Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::RuntimeData) + QStringLiteral("/synth/dssp-logs")));
    }

    void DsspServerLog::rotateLogFile() {
        if (m_file.isOpen()) {
            const auto path = m_file.fileName();
            m_file.close();
            if (QFileInfo(path).size() > 0)
                compressAndArchiveFile(path);
        }
        m_file.setFileName(generateLogFileName());
        if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            setError(tr("Could not open the log file:\n%1").arg(QDir::toNativeSeparators(m_file.fileName())));
            return;
        }
        cleanupOldArchives();
    }

    void DsspServerLog::archiveExistingLogFiles() {
        const QDir directory(logsLocation());
        for (const auto &file : directory.entryInfoList({QStringLiteral("*.log")}, QDir::Files, QDir::Time)) {
            if (file.size() > 0) {
                if (!QFile::exists(file.absoluteFilePath() + QStringLiteral(".gz")))
                    compressAndArchiveFile(file.absoluteFilePath());
            } else {
                QFile::remove(file.absoluteFilePath());
            }
        }
    }

    void DsspServerLog::compressAndArchiveFile(const QString &path) {
        QFile source(path);
        if (!source.open(QIODevice::ReadOnly)) {
            setError(tr("Could not open the log file for compression:\n%1").arg(QDir::toNativeSeparators(path)));
            return;
        }
        const auto data = source.readAll();
        if (source.error() != QFileDevice::NoError) {
            setError(tr("Could not read the log file for compression:\n%1").arg(QDir::toNativeSeparators(path)));
            return;
        }
        source.close();
        const auto compressed = compressData(data, m_configuration.compressLevel);
        if (compressed.isEmpty()) {
            setError(tr("Could not compress the log file:\n%1").arg(QDir::toNativeSeparators(path)));
            return;
        }
        QFile destination(path + QStringLiteral(".gz"));
        if (!destination.open(QIODevice::WriteOnly)) {
            setError(tr("Could not create the log archive:\n%1").arg(QDir::toNativeSeparators(destination.fileName())));
            return;
        }
        if (destination.write(compressed) != compressed.size() || !destination.flush()) {
            destination.remove();
            setError(tr("Could not write to the log archive:\n%1").arg(QDir::toNativeSeparators(destination.fileName())));
            return;
        }
        destination.close();
        QFile::remove(path);
    }

    void DsspServerLog::cleanupOldArchives() const {
        const QDir directory(logsLocation());
        const auto cutoff = QDateTime::currentDateTime().addDays(-m_configuration.maxArchiveDays);
        qint64 size = 0;
        for (const auto &file : directory.entryInfoList({QStringLiteral("*.log.gz")}, QDir::Files, QDir::Time)) {
            if (file.lastModified() < cutoff) {
                QFile::remove(file.absoluteFilePath());
                continue;
            }
            size += file.size();
            if (size > m_configuration.maxArchiveSize)
                QFile::remove(file.absoluteFilePath());
        }
    }

    void DsspServerLog::processLine(const QByteArray &line) {
        const auto document = QJsonDocument::fromJson(line);
        if (!document.isObject())
            return;
        const auto record = document.object();
        const auto level = record.value(QStringLiteral("level")).toString();
        if (level != QStringLiteral("CRITICAL") && level != QStringLiteral("FATAL") && (level != QStringLiteral("WARNING") || !m_configuration.notifyWarnings))
            return;
        const auto module = record.value(QStringLiteral("module"));
        const auto message = record.value(QStringLiteral("message"));
        if (module.isString() && message.isString())
            Q_EMIT recordReceived(level, module.toString(), message.toString());
    }

    void DsspServerLog::writeLine(const QByteArray &line) {
        if (!m_file.isOpen())
            rotateLogFile();
        // Rotate between lines so that a JSON record remains in a single file.
        if (m_file.isOpen() && m_file.size() > m_configuration.maxFileSize)
            rotateLogFile();
        if (!m_file.isOpen())
            return;
        if (m_file.write(line) != line.size() || !m_file.flush()) {
            const auto path = QDir::toNativeSeparators(m_file.fileName());
            m_file.close();
            setError(tr("Could not write to the log file:\n%1").arg(path));
        }
    }

    void DsspServerLog::setError(const QString &message) {
        if (m_errorMessage == message)
            return;
        m_errorMessage = message;
        Q_EMIT errorOccurred(message);
    }

}

#include "moc_DsspServerLog.cpp"
