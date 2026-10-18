// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERLOG_H
#define DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERLOG_H

#include <QByteArray>
#include <QFile>
#include <QObject>
#include <QString>

namespace Synth::Internal {

    struct DsspLogConfiguration {
        qint64 maxFileSize = 16 * 1024 * 1024;
        qint64 maxArchiveSize = 1024LL * 1024 * 1024;
        int maxArchiveDays = 30;
        int compressLevel = 9;
        bool notifyWarnings = false;

        bool operator==(const DsspLogConfiguration &) const = default;
    };

    class DsspServerLog : public QObject {
        Q_OBJECT
    public:
        explicit DsspServerLog(QObject *parent = nullptr);

        void setConfiguration(const DsspLogConfiguration &configuration);
        void beginSession();
        void append(const QByteArray &data);
        void endSession();
        static QString logsLocation();

    Q_SIGNALS:
        void recordReceived(const QString &level, const QString &module, const QString &message);
        void errorOccurred(const QString &message);

    private:
        void rotateLogFile();
        void compressAndArchiveFile(const QString &path);
        void archiveExistingLogFiles();
        void cleanupOldArchives() const;
        void writeLine(const QByteArray &line);
        void processLine(const QByteArray &line);
        void setError(const QString &message);

        DsspLogConfiguration m_configuration;
        QFile m_file;
        QByteArray m_pendingLine;
        QString m_errorMessage;
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERLOG_H
