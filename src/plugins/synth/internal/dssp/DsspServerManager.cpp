// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DsspServerManager.h"

#include <utility>

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QPointer>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/Semver.h>

#include <appinstall/InstallationManager.h>
#include <coreplugin/CoreInterface.h>
#include <coreplugin/NotificationMessage.h>

#include <synth/internal/DsspDeviceCatalog.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcDsspServer, "diffscope.synth.dsspserver")

    namespace {

        QString executableName() {
#ifdef Q_OS_WIN
            return QStringLiteral("dssp.exe");
#else
            return QStringLiteral("dssp");
#endif
        }

    }

    DsspServerManager::DsspServerManager(const QString &pluginDirectory, QObject *parent)
        : QObject(parent), m_process(new QProcess(this)), m_log(new DsspServerLog(this)),
          m_devices(new DsspDeviceCatalog(this)), m_stopTimer(new QTimer(this)) {
        AppInstall::ProductConfiguration product;
        product.id = QStringLiteral("dssp");
        product.displayName = tr("DiffSinger Synthesis Program");
        product.executableRelativePath = QStringLiteral("bin/") + executableName();
        product.bundledExecutablePath = QDir(pluginDirectory).filePath(QStringLiteral("dssp/") + product.executableRelativePath);
        product.downloadedVersionsDirectory = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath(QStringLiteral("dssp/versions"));
        product.catalogUrl = QUrl(QString::fromUtf8(DIFFSCOPE_DSSP_CATALOG_URL));
        product.probeVersion = [](const QString &path, QObject *context, AppInstall::VersionResultHandler finished) -> AppInstall::VersionProbeCancellation {
            auto process = new QProcess(context);
            process->setStandardErrorFile(QProcess::nullDevice());
            QObject::connect(process, &QProcess::errorOccurred, context, [finished](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart)
                    finished({});
            });
            QObject::connect(process, &QProcess::finished, context, [process, finished](int code, QProcess::ExitStatus status) {
                const auto version = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
                finished(code == 0 && status == QProcess::NormalExit && SVS::Semver(version).isValid() ? version : QString{});
            });
            process->start(path, {QStringLiteral("--version")}, QIODevice::ReadOnly);
            return [guard = QPointer<QProcess>(process)] {
                if (!guard)
                    return;
                guard->disconnect();
                if (guard->state() != QProcess::NotRunning) {
                    guard->kill();
                    guard->waitForFinished(1000);
                }
                delete guard.data();
            };
        };
        m_installations = new AppInstall::InstallationManager(product, this);
        connect(m_installations, &AppInstall::InstallationManager::installationsChanged, this, &DsspServerManager::installationNamesChanged);
        connect(m_installations, &AppInstall::InstallationManager::busyChanged, this, &DsspServerManager::stateChanged);
        connect(m_installations, &AppInstall::InstallationManager::selectionChanged, this, [this] {
            const auto selection = m_installations->selection();
            if (m_configuration.installation == selection)
                return;
            m_configuration.installation = selection;
            Q_EMIT configurationChanged();
            Q_EMIT stateChanged();
        });
        m_process->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
            m_log->append(m_process->readAllStandardOutput());
        });
        connect(m_log, &DsspServerLog::recordReceived, this, &DsspServerManager::notifyLog);
        connect(m_log, &DsspServerLog::errorOccurred, this, [this](const QString &message) {
            notifyLog(QStringLiteral("CRITICAL"), QStringLiteral("diffscope.synth.dsspserver"), message);
        });
        m_stopTimer->setSingleShot(true);
        m_stopTimer->setInterval(5000);
        connect(m_stopTimer, &QTimer::timeout, this, [this] {
            if (m_process->state() != QProcess::NotRunning)
                m_process->kill();
        });
        connect(m_process, &QProcess::started, this, [this] {
            qCInfo(lcDsspServer) << "Server process started";
            if (m_stopping)
                m_process->terminate();
            Q_EMIT stateChanged();
        });
        connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error != QProcess::FailedToStart)
                return;
            finishLogging();
            m_stopTimer->stop();
            const bool expected = m_stopping || m_shutdown;
            m_stopping = false;
            const bool restart = std::exchange(m_restartPending, false);
            if (!expected)
                fail(tr("Could not start DiffSinger Synthesis Program:\n%1").arg(m_startExecutable));
            Q_EMIT stateChanged();
            if (restart && !m_shutdown)
                start();
        });
        connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
            finishLogging();
            qCInfo(lcDsspServer) << "Server process exited" << exitCode << exitStatus;
            m_stopTimer->stop();
            const bool expected = m_stopping || m_shutdown;
            m_stopping = false;
            const bool restart = std::exchange(m_restartPending, false);
            if (!expected && (exitStatus == QProcess::CrashExit || exitCode != 0))
                fail(tr("DiffSinger Synthesis Program exited unexpectedly (exit code %L1).").arg(exitCode));
            Q_EMIT stateChanged();
            if (restart && !m_shutdown)
                start();
        });
        connect(m_devices, &DsspDeviceCatalog::finished, this, [this](bool success) {
            if (!m_startPending)
                return;
            m_startPending = false;
            if (!success) {
                fail(m_devices->errorMessage());
                return;
            }
            const auto device = m_devices->resolveDevice(m_startConfiguration.executionProvider, m_startConfiguration.deviceId);
            const auto requestedConfiguration = m_startConfiguration;
            m_startConfiguration.executionProvider = device.value(QStringLiteral("executionProvider")).toString();
            m_startConfiguration.deviceId = device.value(QStringLiteral("id")).toString();
            if (m_configuration == requestedConfiguration)
                setConfiguration(m_startConfiguration);
            launch(device);
        });
    }

    DsspServerManager::~DsspServerManager() {
        shutdown();
    }

    void DsspServerManager::initialize() {
        loadSettings();
        if (m_configuration.autoStart)
            start();
    }

    void DsspServerManager::enableNotifications() {
        m_notificationsReady = true;
        for (const auto &notification : std::exchange(m_pendingLogNotifications, {}))
            notifyLog(notification.level, notification.module, notification.text);
        notifyFailure();
    }

    void DsspServerManager::shutdown() {
        if (m_shutdown)
            return;
        m_shutdown = true;
        m_installations->shutdown();
        m_restartPending = false;
        m_startPending = false;
        m_devices->cancel();
        m_stopTimer->stop();
        m_stopping = true;
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            if (!m_process->waitForFinished(1000)) {
                m_process->kill();
                m_process->waitForFinished(1000);
            }
        }
        finishLogging();
    }

    DsspServerConfiguration DsspServerManager::configuration() const {
        return m_configuration;
    }

    void DsspServerManager::setConfiguration(const DsspServerConfiguration &configuration) {
        auto normalized = configuration;
        normalized.installation = AppInstall::InstallationManager::normalizedSelection(normalized.installation);
        normalized.packageDirectory = normalizedPath(normalized.packageDirectory);
        if (m_configuration == normalized)
            return;
        m_configuration = normalized;
        m_installations->setSelection(m_configuration.installation);
        m_log->setConfiguration(m_configuration.logging);
        saveSettings();
        Q_EMIT configurationChanged();
        Q_EMIT stateChanged();
    }

    DsspServerConfiguration DsspServerManager::defaultConfiguration() const {
        DsspServerConfiguration configuration;
        configuration.installation.id = m_installations->defaultInstallationId();
        return configuration;
    }

    AppInstall::InstallationManager *DsspServerManager::installationManager() const {
        return m_installations;
    }

    QVariantList DsspServerManager::installations(const DsspServerConfiguration &configuration) const {
        return m_installations->installations(configuration.installation);
    }

    void DsspServerManager::refreshInstallationVersions(const DsspServerConfiguration &configuration) {
        m_installations->refreshVersions(configuration.installation);
    }

    QString DsspServerManager::downloadInstallation(QWindow *parent) {
        return m_shutdown ? QString{} : m_installations->download(parent).id;
    }

    bool DsspServerManager::uninstallInstallation(const QString &id, QWindow *parent) {
        if (m_shutdown)
            return false;
        auto selection = m_configuration.installation;
        selection.id = id;
        const bool inUse = m_startExecutable == m_installations->executablePath(selection)
                           && (m_startPending || m_process->state() != QProcess::NotRunning);
        return m_installations->uninstall(id, parent, [this](const QString &directory, QString *error) {
            if (!AppInstall::InstallationManager::pathIsWithin(directory, m_startExecutable)
                || (!m_startPending && m_process->state() == QProcess::NotRunning))
                return true;
            stop();
            if (!m_devices->cancelAndWait()) {
                *error = tr("Could not stop device discovery.");
                return false;
            }
            if (m_process->state() != QProcess::NotRunning && !m_process->waitForFinished(5000)) {
                m_process->kill();
                m_process->waitForFinished(1000);
            }
            if (m_process->state() != QProcess::NotRunning) {
                *error = tr("Could not stop DiffSinger Synthesis Program.");
                return false;
            }
            return true;
        }, inUse ? tr("DiffSinger Synthesis Program will be stopped before this version is uninstalled.") : QString{});
    }

    QString DsspServerManager::executablePath() const {
        return m_installations->executablePath(m_configuration.installation);
    }

    QString DsspServerManager::executablePath(const DsspServerConfiguration &configuration) const {
        return m_installations->executablePath(configuration.installation);
    }

    QString DsspServerManager::normalizedPath(const QString &path) {
        return AppInstall::InstallationManager::normalizedPath(path);
    }

    bool DsspServerManager::running() const {
        return m_process->state() == QProcess::Running;
    }

    bool DsspServerManager::canStart() const {
        return !m_shutdown && !m_installations->busy() && !m_stopping && !m_startPending && m_process->state() == QProcess::NotRunning && !executablePath().isEmpty();
    }

    bool DsspServerManager::canStop() const {
        return !m_shutdown && !m_stopping && (m_startPending || m_process->state() != QProcess::NotRunning);
    }

    bool DsspServerManager::canRestart() const {
        return !m_shutdown && !m_installations->busy() && !m_stopping && !executablePath().isEmpty();
    }

    QString DsspServerManager::statusIcon() const {
        if (executablePath().isEmpty())
            return QStringLiteral("dssp_question_mark");
        if (running())
            return QStringLiteral("dssp");
        return m_lastRunFailed ? QStringLiteral("dssp_error") : QStringLiteral("dssp_off");
    }

    QString DsspServerManager::statusMenuText() const {
        if (executablePath().isEmpty())
            return tr("Not Configured");
        if (m_stopping)
            return tr("Stopping");
        if (m_startPending || m_process->state() == QProcess::Starting)
            return tr("Starting");
        if (running())
            return tr("Running");
        return m_lastRunFailed ? tr("Error") : tr("Stopped");
    }

    QString DsspServerManager::statusText() const {
        if (executablePath().isEmpty())
            return tr("DiffSinger Synthesis Program is not configured");
        if (m_stopping)
            return tr("DiffSinger Synthesis Program is stopping");
        if (m_startPending || m_process->state() == QProcess::Starting)
            return tr("DiffSinger Synthesis Program is starting");
        if (running())
            return tr("DiffSinger Synthesis Program is running");
        return m_lastRunFailed ? tr("DiffSinger Synthesis Program exited abnormally") : tr("DiffSinger Synthesis Program is stopped");
    }

    void DsspServerManager::loadSettings() {
        m_configuration = defaultConfiguration();
        m_configuration.installation = m_installations->selection();
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        m_configuration.autoStart = settings->value(QStringLiteral("autoStart"), true).toBool();
        m_configuration.host = settings->value(QStringLiteral("host"), m_configuration.host).toString();
        m_configuration.port = settings->value(QStringLiteral("port"), m_configuration.port).toInt();
        m_configuration.packageDirectory = normalizedPath(settings->value(QStringLiteral("packageDirectory")).toString());
        m_configuration.logLevel = settings->value(QStringLiteral("logLevel"), m_configuration.logLevel).toString();
        m_configuration.useDefaultDevice = settings->value(QStringLiteral("useDefaultDevice"), true).toBool();
        m_configuration.executionProvider = settings->value(QStringLiteral("executionProvider")).toString();
        m_configuration.deviceId = settings->value(QStringLiteral("deviceId")).toString();
        m_configuration.extraArguments = settings->value(QStringLiteral("extraArguments")).toString();
        m_configuration.logging.maxFileSize = qMax(1LL, settings->value(QStringLiteral("logMaxFileSize"), m_configuration.logging.maxFileSize).toLongLong());
        m_configuration.logging.maxArchiveSize = qMax(1LL, settings->value(QStringLiteral("logMaxArchiveSize"), m_configuration.logging.maxArchiveSize).toLongLong());
        m_configuration.logging.maxArchiveDays = qMax(1, settings->value(QStringLiteral("logMaxArchiveDays"), m_configuration.logging.maxArchiveDays).toInt());
        m_configuration.logging.compressLevel = qBound(0, settings->value(QStringLiteral("logCompressLevel"), m_configuration.logging.compressLevel).toInt(), 9);
        m_configuration.logging.notifyWarnings = settings->value(QStringLiteral("notifyWarnings"), m_configuration.logging.notifyWarnings).toBool();
        settings->endGroup();
        m_log->setConfiguration(m_configuration.logging);
    }

    void DsspServerManager::saveSettings() const {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("autoStart"), m_configuration.autoStart);
        settings->setValue(QStringLiteral("host"), m_configuration.host);
        settings->setValue(QStringLiteral("port"), m_configuration.port);
        settings->setValue(QStringLiteral("packageDirectory"), m_configuration.packageDirectory);
        settings->setValue(QStringLiteral("logLevel"), m_configuration.logLevel);
        settings->setValue(QStringLiteral("useDefaultDevice"), m_configuration.useDefaultDevice);
        settings->setValue(QStringLiteral("executionProvider"), m_configuration.executionProvider);
        settings->setValue(QStringLiteral("deviceId"), m_configuration.deviceId);
        settings->setValue(QStringLiteral("extraArguments"), m_configuration.extraArguments);
        settings->setValue(QStringLiteral("logMaxFileSize"), m_configuration.logging.maxFileSize);
        settings->setValue(QStringLiteral("logMaxArchiveSize"), m_configuration.logging.maxArchiveSize);
        settings->setValue(QStringLiteral("logMaxArchiveDays"), m_configuration.logging.maxArchiveDays);
        settings->setValue(QStringLiteral("logCompressLevel"), m_configuration.logging.compressLevel);
        settings->setValue(QStringLiteral("notifyWarnings"), m_configuration.logging.notifyWarnings);
        settings->endGroup();
    }

    void DsspServerManager::start() {
        if (!canStart())
            return;
        m_lastRunFailed = false;
        m_failureMessage.clear();
        m_startConfiguration = m_configuration;
        m_startExecutable = executablePath();
        if (!QFileInfo(m_startExecutable).isFile()) {
            fail(tr("DiffSinger Synthesis Program was not found:\n%1").arg(m_startExecutable));
            return;
        }
        if (m_startConfiguration.useDefaultDevice) {
            launch();
        } else {
            m_startPending = true;
            Q_EMIT stateChanged();
            m_devices->refresh(m_startExecutable);
        }
    }

    void DsspServerManager::launch(const QVariantMap &device) {
        QStringList arguments{
            QStringLiteral("serve"),
            QStringLiteral("--host"), m_startConfiguration.host,
            QStringLiteral("--port"), QString::number(m_startConfiguration.port),
            QStringLiteral("--log-level"), m_startConfiguration.logLevel,
        };
        if (!m_startConfiguration.packageDirectory.isEmpty())
            arguments.append({QStringLiteral("--package-dir"), m_startConfiguration.packageDirectory});
        if (!m_startConfiguration.useDefaultDevice) {
            arguments.append({
                QStringLiteral("--execution-provider"), device.value(QStringLiteral("executionProvider")).toString(),
                QStringLiteral("--device-index"), QString::number(device.value(QStringLiteral("index")).toInt()),
            });
        }
        arguments.append(QProcess::splitCommand(m_startConfiguration.extraArguments));
        arguments.append({QStringLiteral("--log-mode"), QStringLiteral("structured")});
        m_log->beginSession();
        qCInfo(lcDsspServer) << "Starting server process";
        m_process->start(m_startExecutable, arguments, QIODevice::ReadOnly);
        Q_EMIT stateChanged();
    }

    void DsspServerManager::stop() {
        stopProcess(false);
    }

    void DsspServerManager::restart() {
        if (m_installations->busy())
            return;
        if (!m_shutdown && m_stopping && !executablePath().isEmpty()) {
            m_restartPending = true;
            return;
        }
        if (canRestart())
            stopProcess(true);
    }

    void DsspServerManager::stopProcess(bool restart) {
        if (m_shutdown)
            return;
        m_restartPending = restart;
        m_startPending = false;
        m_devices->cancel();
        if (m_process->state() == QProcess::NotRunning) {
            m_stopping = false;
            m_restartPending = false;
            Q_EMIT stateChanged();
            if (restart)
                start();
            return;
        }
        m_stopping = true;
        m_process->terminate();
        m_stopTimer->start();
        Q_EMIT stateChanged();
    }

    void DsspServerManager::fail(const QString &message) {
        qCWarning(lcDsspServer) << "Server operation failed" << message;
        m_lastRunFailed = true;
        m_failureMessage = message;
        Q_EMIT stateChanged();
        notifyFailure();
    }

    void DsspServerManager::notifyFailure() {
        if (!m_notificationsReady || m_shutdown || m_failureMessage.isEmpty())
            return;
        auto message = new Core::NotificationMessage(this);
        message->setIcon(SVS::SVSCraft::Critical);
        message->setTitle(tr("DiffSinger Synthesis Program Stopped"));
        message->setText(std::exchange(m_failureMessage, {}));
        message->setTextFormat(Qt::PlainText);
        message->setButtons({tr("Restart")});
        message->setPrimaryButton(0);
        message->setClosable(true);
        connect(message, &Core::NotificationMessage::buttonClicked, this, [this, message](int index) {
            if (index == 0) {
                message->close();
                restart();
            }
        });
        connect(message, &Core::NotificationMessage::closed, message, &QObject::deleteLater);
        Core::CoreInterface::sendNotification(message);
    }

    void DsspServerManager::notifyLog(const QString &level, const QString &module, const QString &text) {
        if (m_shutdown)
            return;
        if (!m_notificationsReady) {
            m_pendingLogNotifications.append({level, module, text});
            return;
        }
        QString title;
        if (level == QStringLiteral("WARNING"))
            title = tr("DiffSinger Synthesis Program Warning");
        else if (level == QStringLiteral("CRITICAL"))
            title = tr("DiffSinger Synthesis Program Critical");
        else if (level == QStringLiteral("FATAL"))
            title = tr("DiffSinger Synthesis Program Fatal");
        else
            return;
        auto message = new Core::NotificationMessage(this);
        message->setIcon(level == QStringLiteral("WARNING") ? SVS::SVSCraft::Warning : SVS::SVSCraft::Critical);
        message->setTitle(title);
        message->setText(tr("Module: %1\n%2").arg(module, text));
        message->setTextFormat(Qt::PlainText);
        message->setClosable(true);
        connect(message, &Core::NotificationMessage::closed, message, &QObject::deleteLater);
        Core::CoreInterface::sendNotification(message);
    }

    void DsspServerManager::finishLogging() {
        m_log->append(m_process->readAllStandardOutput());
        m_log->endSession();
    }

}

#include "moc_DsspServerManager.cpp"
