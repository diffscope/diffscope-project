// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DsspServerPage.h"

#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <appinstall/InstallationManager.h>

#include <synth/internal/DsspDeviceCatalog.h>
#include <synth/internal/DsspServerLog.h>

namespace Synth::Internal {

    DsspServerPage::DsspServerPage(DsspServerManager *manager, QObject *parent)
        : Core::ISettingPage(QStringLiteral("org.diffscope.synth.DsspServer"), parent),
          m_manager(manager), m_devices(new DsspDeviceCatalog(this)), m_configuration(manager->configuration()) {
        setTitle(tr("DiffSinger"));
        setDescription(tr("Manager DiffSinger Synthesis Program and startup options"));
        connect(m_manager, &DsspServerManager::installationNamesChanged, this, &DsspServerPage::installationsChanged);
        connect(m_manager, &DsspServerManager::stateChanged, this, &DsspServerPage::busyChanged);
        connect(m_devices, &DsspDeviceCatalog::changed, this, &DsspServerPage::configurationChanged);
        connect(m_devices, &DsspDeviceCatalog::finished, this, [this](bool success) {
            if (!success)
                return;
            const auto device = m_devices->resolveDevice(m_configuration.executionProvider, m_configuration.deviceId);
            m_configuration.executionProvider = device.value(QStringLiteral("executionProvider")).toString();
            m_configuration.deviceId = device.value(QStringLiteral("id")).toString();
            Q_EMIT configurationChanged();
        });
    }

    DsspServerPage::~DsspServerPage() {
        delete m_widget;
    }

    QString DsspServerPage::sortKeyword() const {
        return QStringLiteral("03DsspServer");
    }

    bool DsspServerPage::matches(const QString &word) {
        if (Core::ISettingPage::matches(word))
            return true;
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool result{};
        return matcher && QMetaObject::invokeMethod(matcher, "matches", qReturnArg(result), word) && result;
    }

    QObject *DsspServerPage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("DsspServerPage"));
        if (component.isError())
            qFatal() << component.errorString();
        m_widget = component.createWithInitialProperties({
            {QStringLiteral("pageHandle"), QVariant::fromValue(this)},
        });
        if (!m_widget)
            qFatal() << component.errorString();
        m_widget->setParent(this);
        return m_widget;
    }

    void DsspServerPage::beginSetting() {
        m_configuration = m_manager->configuration();
        setErrorMessage({});
        Q_EMIT installationsChanged();
        Q_EMIT configurationChanged();
        refreshDevices();
        m_manager->refreshInstallationVersions(m_configuration);
        Core::ISettingPage::beginSetting();
    }

    bool DsspServerPage::accept() {
        if (busy())
            return false;
        if (m_configuration.host.trimmed().isEmpty()) {
            setErrorMessage(tr("The host must not be empty."));
            return false;
        }
        if (m_configuration.port < 1 || m_configuration.port > 65535) {
            setErrorMessage(tr("The port must be between %L1 and %L2.").arg(1).arg(65535));
            return false;
        }
        auto previous = m_manager->configuration();
        auto current = m_configuration;
        const bool pathChanged = m_manager->executablePath(previous) != m_manager->executablePath(m_configuration);
        previous.autoStart = m_configuration.autoStart;
        previous.logging = current.logging;
        previous.installation = current.installation;
        if (previous.useDefaultDevice && current.useDefaultDevice) {
            previous.executionProvider.clear();
            previous.deviceId.clear();
            current.executionProvider.clear();
            current.deviceId.clear();
        }
        const bool startupChanged = pathChanged || previous != current;
        m_manager->setConfiguration(m_configuration);
        setErrorMessage({});
        if (startupChanged && m_manager->canStop()) {
            auto item = qobject_cast<QQuickItem *>(widget());
            const bool stop = m_manager->executablePath().isEmpty();
            if (SVS::MessageBox::question(
                    Core::RuntimeInterface::qmlEngine(), item ? item->window() : nullptr,
                    stop ? tr("Stop DiffSinger Synthesis Program") : tr("Restart DiffSinger Synthesis Program"),
                    stop ? tr("No installation is selected. Stop DiffSinger Synthesis Program now?")
                         : tr("The startup options have changed. Restart DiffSinger Synthesis Program now?"),
                    SVS::SVSCraft::Yes | SVS::SVSCraft::No, SVS::SVSCraft::No
                ) == SVS::SVSCraft::Yes) {
                if (stop)
                    m_manager->stop();
                else
                    m_manager->restart();
            }
        }
        return Core::ISettingPage::accept();
    }

    void DsspServerPage::endSetting() {
        m_devices->cancel();
        Core::ISettingPage::endSetting();
    }

    QVariantMap DsspServerPage::configuration() const {
        return {
            {QStringLiteral("installationId"), m_configuration.installation.id},
            {QStringLiteral("executablePath"), m_manager->executablePath(m_configuration)},
            {QStringLiteral("autoStart"), m_configuration.autoStart},
            {QStringLiteral("host"), m_configuration.host},
            {QStringLiteral("port"), m_configuration.port},
            {QStringLiteral("logLevel"), m_configuration.logLevel},
            {QStringLiteral("useDefaultDevice"), m_configuration.useDefaultDevice},
            {QStringLiteral("executionProvider"), m_configuration.executionProvider},
            {QStringLiteral("deviceId"), m_configuration.deviceId},
            {QStringLiteral("extraArguments"), m_configuration.extraArguments},
            {QStringLiteral("logMaxFileSize"), m_configuration.logging.maxFileSize},
            {QStringLiteral("logMaxArchiveSize"), m_configuration.logging.maxArchiveSize},
            {QStringLiteral("logMaxArchiveDays"), m_configuration.logging.maxArchiveDays},
            {QStringLiteral("logCompressLevel"), m_configuration.logging.compressLevel},
            {QStringLiteral("notifyWarnings"), m_configuration.logging.notifyWarnings},
        };
    }

    DsspServerConfiguration DsspServerPage::serverConfiguration() const {
        return m_configuration;
    }

    QVariantList DsspServerPage::installations() const {
        return m_manager->installations(m_configuration);
    }

    QVariantMap DsspServerPage::selectedDevice() const {
        return m_configuration.useDefaultDevice ? m_devices->defaultDevice()
                                                : m_devices->resolveDevice(m_configuration.executionProvider, m_configuration.deviceId);
    }

    QObject *DsspServerPage::deviceCatalog() const {
        return m_devices;
    }

    QString DsspServerPage::errorMessage() const {
        return m_errorMessage;
    }

    QString DsspServerPage::logsLocation() const {
        return DsspServerLog::logsLocation();
    }

    bool DsspServerPage::busy() const {
        return m_manager->packageOperationActive();
    }

    void DsspServerPage::setValue(const QString &key, const QVariant &value) {
        if (busy())
            return;
        const auto previous = m_configuration;
        if (key == QStringLiteral("installationId"))
            m_configuration.installation.id = value.toString();
        else if (key == QStringLiteral("autoStart"))
            m_configuration.autoStart = value.toBool();
        else if (key == QStringLiteral("host"))
            m_configuration.host = value.toString();
        else if (key == QStringLiteral("port"))
            m_configuration.port = value.toInt();
        else if (key == QStringLiteral("logLevel"))
            m_configuration.logLevel = value.toString();
        else if (key == QStringLiteral("useDefaultDevice"))
            m_configuration.useDefaultDevice = value.toBool();
        else if (key == QStringLiteral("executionProvider")) {
            m_configuration.executionProvider = value.toString();
            const auto devices = m_devices->devices(m_configuration.executionProvider);
            if (!devices.isEmpty())
                m_configuration.deviceId = devices.first().toMap().value(QStringLiteral("id")).toString();
        } else if (key == QStringLiteral("deviceId"))
            m_configuration.deviceId = value.toString();
        else if (key == QStringLiteral("extraArguments"))
            m_configuration.extraArguments = value.toString();
        else if (key == QStringLiteral("logMaxFileSize"))
            m_configuration.logging.maxFileSize = qMax(1LL, value.toLongLong());
        else if (key == QStringLiteral("logMaxArchiveSize"))
            m_configuration.logging.maxArchiveSize = qMax(1LL, value.toLongLong());
        else if (key == QStringLiteral("logMaxArchiveDays"))
            m_configuration.logging.maxArchiveDays = qBound(1, value.toInt(), 3650);
        else if (key == QStringLiteral("logCompressLevel"))
            m_configuration.logging.compressLevel = qBound(0, value.toInt(), 9);
        else if (key == QStringLiteral("notifyWarnings"))
            m_configuration.logging.notifyWarnings = value.toBool();
        if (previous == m_configuration)
            return;
        setErrorMessage({});
        Q_EMIT configurationChanged();
        markDirty();
        if (m_manager->executablePath(previous) != m_manager->executablePath(m_configuration))
            refreshDevices();
    }

    void DsspServerPage::addLocalInstallation(const QUrl &url) {
        if (busy())
            return;
        QString error;
        if (!m_manager->installationManager()->addLocalInstallation(m_configuration.installation, localFilePath(url), &error)) {
            setErrorMessage(error);
            return;
        }
        setErrorMessage({});
        Q_EMIT installationsChanged();
        Q_EMIT configurationChanged();
        markDirty();
        refreshDevices();
        m_manager->refreshInstallationVersions(m_configuration);
    }

    void DsspServerPage::removeLocalInstallation() {
        if (busy())
            return;
        if (!m_manager->installationManager()->removeLocalInstallation(m_configuration.installation))
            return;
        Q_EMIT installationsChanged();
        Q_EMIT configurationChanged();
        markDirty();
        refreshDevices();
        m_manager->refreshInstallationVersions(m_configuration);
    }

    void DsspServerPage::refreshDevices() {
        if (busy())
            return;
        m_devices->refresh(m_manager->executablePath(m_configuration));
    }

    void DsspServerPage::downloadInstallation() {
        auto item = qobject_cast<QQuickItem *>(widget());
        const auto id = m_manager->downloadInstallation(item ? item->window() : nullptr);
        if (!id.isEmpty())
            setValue(QStringLiteral("installationId"), id);
    }

    void DsspServerPage::uninstallInstallation() {
        if (busy() || !m_configuration.installation.id.startsWith(QStringLiteral("downloaded:")))
            return;
        const auto id = m_configuration.installation.id;
        if (!m_devices->cancelAndWait()) {
            setErrorMessage(tr("Could not stop device discovery."));
            return;
        }
        auto item = qobject_cast<QQuickItem *>(widget());
        if (m_manager->uninstallInstallation(id, item ? item->window() : nullptr))
            setValue(QStringLiteral("installationId"), m_manager->defaultConfiguration().installation.id);
        refreshDevices();
    }

    QString DsspServerPage::localFilePath(const QUrl &url) const {
        return url.isLocalFile() ? DsspServerManager::normalizedPath(url.toLocalFile()) : QString{};
    }

    void DsspServerPage::setErrorMessage(const QString &message) {
        if (m_errorMessage == message)
            return;
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }

}

#include "moc_DsspServerPage.cpp"
