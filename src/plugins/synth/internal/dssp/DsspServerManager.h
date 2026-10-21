// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERMANAGER_H
#define DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERMANAGER_H

#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>

#include <appinstall/InstallationTypes.h>

#include <synth/internal/DsspServerLog.h>

class QProcess;
class QTimer;
class QWindow;

namespace AppInstall {
    class InstallationManager;
}

namespace Synth::Internal {

    class DsspDeviceCatalog;

    struct DsspServerConfiguration {
        AppInstall::InstallationSelection installation;
        bool autoStart = true;
        QString host = QStringLiteral("127.0.0.1");
        int port = 13711;
        QString packageDirectory;
        QString logLevel = QStringLiteral("info");
        bool useDefaultDevice = true;
        QString executionProvider;
        QString deviceId;
        QString extraArguments;
        DsspLogConfiguration logging;

        bool operator==(const DsspServerConfiguration &) const = default;
    };

    class DsspServerManager : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString executablePath READ executablePath NOTIFY configurationChanged)
        Q_PROPERTY(bool running READ running NOTIFY stateChanged)
        Q_PROPERTY(bool canStart READ canStart NOTIFY stateChanged)
        Q_PROPERTY(bool canStop READ canStop NOTIFY stateChanged)
        Q_PROPERTY(bool canRestart READ canRestart NOTIFY stateChanged)
        Q_PROPERTY(QString statusIcon READ statusIcon NOTIFY stateChanged)
        Q_PROPERTY(QString statusMenuText READ statusMenuText NOTIFY stateChanged)
        Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    public:
        explicit DsspServerManager(const QString &pluginDirectory, QObject *parent = nullptr);
        ~DsspServerManager() override;

        void initialize();
        void enableNotifications();
        void shutdown();

        DsspServerConfiguration configuration() const;
        void setConfiguration(const DsspServerConfiguration &configuration);
        DsspServerConfiguration defaultConfiguration() const;
        AppInstall::InstallationManager *installationManager() const;
        QVariantList installations(const DsspServerConfiguration &configuration) const;
        void refreshInstallationVersions(const DsspServerConfiguration &configuration);
        QString executablePath() const;
        QString executablePath(const DsspServerConfiguration &configuration) const;
        static QString normalizedPath(const QString &path);
        QString downloadInstallation(QWindow *parent);
        bool uninstallInstallation(const QString &id, QWindow *parent);

        bool running() const;
        bool canStart() const;
        bool canStop() const;
        bool canRestart() const;
        QString statusIcon() const;
        QString statusMenuText() const;
        QString statusText() const;

    public Q_SLOTS:
        void start();
        void stop();
        void restart();

    Q_SIGNALS:
        void configurationChanged();
        void installationNamesChanged();
        void stateChanged();

    private:
        void loadSettings();
        void saveSettings() const;
        void launch(const QVariantMap &device = {});
        void stopProcess(bool restart);
        void fail(const QString &message);
        void notifyFailure();
        void notifyLog(const QString &level, const QString &module, const QString &text);
        void finishLogging();

        struct LogNotification {
            QString level;
            QString module;
            QString text;
        };

        AppInstall::InstallationManager *m_installations{};
        DsspServerConfiguration m_configuration;
        DsspServerConfiguration m_startConfiguration;
        QString m_startExecutable;
        QProcess *m_process{};
        DsspServerLog *m_log{};
        DsspDeviceCatalog *m_devices{};
        QTimer *m_stopTimer{};
        bool m_startPending{};
        bool m_stopping{};
        bool m_restartPending{};
        bool m_lastRunFailed{};
        bool m_shutdown{};
        bool m_notificationsReady{};
        QString m_failureMessage;
        QList<LogNotification> m_pendingLogNotifications;
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERMANAGER_H
