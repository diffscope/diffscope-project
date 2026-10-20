// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DsspServerManager.h"

#include <utility>

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/Semver.h>

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
        : QObject(parent), m_pluginDirectory(pluginDirectory), m_process(new QProcess(this)), m_log(new DsspServerLog(this)),
          m_devices(new DsspDeviceCatalog(this)), m_stopTimer(new QTimer(this)),
          m_versionProcess(new QProcess(this)), m_versionTimer(new QTimer(this)) {
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
        m_versionProcess->setStandardErrorFile(QProcess::nullDevice());
        m_versionTimer->setSingleShot(true);
        m_versionTimer->setInterval(5000);
        connect(m_versionTimer, &QTimer::timeout, this, [this] {
            m_versionTimedOut = true;
            m_versionProcess->kill();
        });
        connect(m_versionProcess, &QProcess::started, this, [this] {
            if (m_shutdown || m_versionTimedOut)
                m_versionProcess->kill();
        });
        connect(m_versionProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart)
                completeVersionQuery({});
        });
        connect(m_versionProcess, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
            const auto output = m_versionProcess->readAllStandardOutput();
            const bool success = !m_versionTimedOut && status == QProcess::NormalExit && exitCode == 0;
            completeVersionQuery(success ? QString::fromUtf8(output).trimmed() : QString{});
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
        m_pendingVersionPaths.clear();
        m_versionTimer->stop();
        if (m_versionProcess->state() != QProcess::NotRunning) {
            m_versionProcess->kill();
            m_versionProcess->waitForFinished(1000);
        }
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
        for (auto &value : normalized.localInstallations) {
            auto installation = value.toMap();
            installation.insert(QStringLiteral("path"), normalizedPath(installation.value(QStringLiteral("path")).toString()));
            value = installation;
        }
        normalized.packageDirectory = normalizedPath(normalized.packageDirectory);
        if (m_configuration == normalized)
            return;
        m_configuration = normalized;
        m_log->setConfiguration(m_configuration.logging);
        saveSettings();
        Q_EMIT configurationChanged();
        Q_EMIT stateChanged();
    }

    DsspServerConfiguration DsspServerManager::defaultConfiguration() const {
        DsspServerConfiguration configuration;
        if (!bundledExecutablePath().isEmpty())
            configuration.installationId = QStringLiteral("bundled");
        return configuration;
    }

    QVariantList DsspServerManager::installations(const DsspServerConfiguration &configuration) const {
        QVariantList result{
            QVariantMap{
                {QStringLiteral("id"), QString{}},
                {QStringLiteral("name"), tr("No installation selected")},
                {QStringLiteral("path"), QString{}},
            },
        };
        if (!bundledExecutablePath().isEmpty()) {
            result.append(QVariantMap{
                {QStringLiteral("id"), QStringLiteral("bundled")},
                {QStringLiteral("name"), tr("Built-in")},
                {QStringLiteral("path"), bundledExecutablePath()},
            });
        }
        const QDir root(downloadedVersionsDirectory());
        for (const auto &directory : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            const auto path = QDir(directory.absoluteFilePath()).filePath(QStringLiteral("bin/") + executableName());
            if (!QFileInfo(path).isFile())
                continue;
            result.append(QVariantMap{
                {QStringLiteral("id"), QStringLiteral("downloaded:") + directory.fileName()},
                {QStringLiteral("name"), tr("Downloaded %1").arg(directory.fileName())},
                {QStringLiteral("path"), normalizedPath(path)},
            });
        }
        for (const auto &value : configuration.localInstallations) {
            const auto installation = value.toMap();
            const auto path = normalizedPath(installation.value(QStringLiteral("path")).toString());
            const auto version = m_installationVersions.value(path);
            result.append(QVariantMap{
                {QStringLiteral("id"), installation.value(QStringLiteral("id"))},
                {QStringLiteral("name"), version.isEmpty() ? tr("DiffSinger Synthesis Program (version unknown)") : tr("DiffSinger Synthesis Program %1").arg(version)},
                {QStringLiteral("path"), path},
            });
        }
        const auto &id = configuration.installationId;
        bool found = false;
        for (const auto &value : std::as_const(result))
            found |= value.toMap().value(QStringLiteral("id")).toString() == id;
        if (!found) {
            result.prepend(QVariantMap{
                {QStringLiteral("id"), id},
                {QStringLiteral("name"), tr("Selected installation (unavailable)")},
                {QStringLiteral("path"), QString{}},
            });
        }
        return result;
    }

    void DsspServerManager::refreshInstallationVersions(const DsspServerConfiguration &configuration) {
        if (m_shutdown)
            return;
        m_pendingVersionPaths.clear();
        for (const auto &value : configuration.localInstallations) {
            const auto path = normalizedPath(value.toMap().value(QStringLiteral("path")).toString());
            if (!path.isEmpty() && path != m_versionPath && !m_pendingVersionPaths.contains(path))
                m_pendingVersionPaths.append(path);
        }
        startNextVersionQuery();
    }

    void DsspServerManager::startNextVersionQuery() {
        if (m_shutdown || !m_versionPath.isEmpty() || m_pendingVersionPaths.isEmpty() || m_versionProcess->state() != QProcess::NotRunning)
            return;
        m_versionPath = m_pendingVersionPaths.takeFirst();
        m_versionTimedOut = false;
        m_versionTimer->start();
        m_versionProcess->start(m_versionPath, {QStringLiteral("--version")}, QIODevice::ReadOnly);
    }

    void DsspServerManager::completeVersionQuery(const QString &version) {
        m_versionTimer->stop();
        const auto path = std::exchange(m_versionPath, {});
        if (path.isEmpty() || m_shutdown)
            return;
        const auto acceptedVersion = SVS::Semver(version).isValid() ? version : QString{};
        if (!m_installationVersions.contains(path) || m_installationVersions.value(path) != acceptedVersion) {
            m_installationVersions.insert(path, acceptedVersion);
            Q_EMIT installationNamesChanged();
        }
        QTimer::singleShot(0, this, &DsspServerManager::startNextVersionQuery);
    }

    QString DsspServerManager::downloadedVersionsDirectory() const {
        return normalizedPath(QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath(QStringLiteral("dssp/versions")));
    }

    QString DsspServerManager::executablePath() const {
        return executablePath(m_configuration);
    }

    QString DsspServerManager::executablePath(const DsspServerConfiguration &configuration) const {
        if (configuration.installationId == QStringLiteral("bundled"))
            return bundledExecutablePath();
        if (configuration.installationId.startsWith(QStringLiteral("downloaded:"))) {
            const auto version = configuration.installationId.sliced(11);
            if (version.isEmpty() || version == QStringLiteral(".") || version == QStringLiteral("..") || version.contains(u'/') || version.contains(u'\\') || version.contains(u':'))
                return {};
            return normalizedPath(QDir(downloadedVersionsDirectory()).filePath(version + QStringLiteral("/bin/") + executableName()));
        }
        for (const auto &value : configuration.localInstallations) {
            const auto installation = value.toMap();
            if (installation.value(QStringLiteral("id")).toString() == configuration.installationId)
                return normalizedPath(installation.value(QStringLiteral("path")).toString());
        }
        return {};
    }

    QString DsspServerManager::normalizedPath(const QString &path) {
        if (path.trimmed().isEmpty())
            return {};
        const QFileInfo info(QDir::fromNativeSeparators(path.trimmed()));
        const auto canonical = info.canonicalFilePath();
        return QDir::toNativeSeparators(canonical.isEmpty() ? QDir::cleanPath(info.absoluteFilePath()) : canonical);
    }

    bool DsspServerManager::running() const {
        return m_process->state() == QProcess::Running;
    }

    bool DsspServerManager::canStart() const {
        return !m_shutdown && !m_stopping && !m_startPending && m_process->state() == QProcess::NotRunning && !executablePath().isEmpty();
    }

    bool DsspServerManager::canStop() const {
        return !m_shutdown && !m_stopping && (m_startPending || m_process->state() != QProcess::NotRunning);
    }

    bool DsspServerManager::canRestart() const {
        return !m_shutdown && !m_stopping && !executablePath().isEmpty();
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

    QString DsspServerManager::bundledExecutablePath() const {
        const auto path = QDir(m_pluginDirectory).filePath(QStringLiteral("dssp/bin/") + executableName());
        return QFileInfo(path).isFile() ? normalizedPath(path) : QString{};
    }

    void DsspServerManager::loadSettings() {
        m_configuration = defaultConfiguration();
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        m_configuration.installationId = settings->value(QStringLiteral("installationId"), m_configuration.installationId).toString();
        m_configuration.localInstallations = settings->value(QStringLiteral("localInstallations")).toList();
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
        settings->setValue(QStringLiteral("installationId"), m_configuration.installationId);
        settings->setValue(QStringLiteral("localInstallations"), m_configuration.localInstallations);
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
