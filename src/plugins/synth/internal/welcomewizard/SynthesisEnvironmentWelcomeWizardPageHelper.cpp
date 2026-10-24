// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisEnvironmentWelcomeWizardPageHelper.h"

#include <algorithm>

#include <QAbstractItemModel>
#include <QScopedValueRollback>
#include <QStringList>

#include <CoreApi/runtimeinterface.h>

#include <synth/internal/DsspServerManager.h>
#include <synth/internal/DsspServerPage.h>
#include <synth/internal/ServiceConfigurationModel.h>
#include <synth/internal/SynthService.h>

namespace Synth::Internal {

    SynthesisEnvironmentWelcomeWizardPageHelper::SynthesisEnvironmentWelcomeWizardPageHelper(QObject *parent)
        : QObject(parent),
          m_server(Core::RuntimeInterface::instance()->getFirstObject<DsspServerManager>()),
          m_localSettings(new DsspServerPage(m_server, this)),
          m_service(SynthService::instance()),
          m_serviceModel(new ServiceConfigurationModel(this)) {
        reloadServiceConfigurations();
        connect(m_serviceModel, &ServiceConfigurationModel::edited, this, [this] {
            m_serviceEdited = true;
            const auto configurations = m_serviceModel->configurations();
            if (std::ranges::all_of(configurations, [](const auto &configuration) { return configuration.validate(); }))
                m_lastValidServices = configurations;
            setErrorMessage({});
        });
        connect(m_localSettings, &DsspServerPage::configurationChanged, this, &SynthesisEnvironmentWelcomeWizardPageHelper::applyLocalConfiguration);
        connect(m_server, &DsspServerManager::configurationChanged, this, [this] {
            Q_EMIT useLocalChanged();
            if (!m_applyingLocal && m_active)
                reloadLocalSettings();
        });
        connect(m_service, &SynthService::serviceDetailsChanged, this, [this](const QUuid &id) {
            if (id == m_currentServiceId)
                Q_EMIT connectionStatusChanged();
        });
        connect(m_service, &SynthService::serviceConfigurationsChanged, this, [this] {
            if (!m_serviceEdited)
                reloadServiceConfigurations();
            Q_EMIT connectionStatusChanged();
        });
    }

    bool SynthesisEnvironmentWelcomeWizardPageHelper::useLocal() const {
        return m_server->configuration().autoStart;
    }

    void SynthesisEnvironmentWelcomeWizardPageHelper::setUseLocal(bool useLocal) {
        auto configuration = m_server->configuration();
        if (configuration.autoStart == useLocal)
            return;
        configuration.autoStart = useLocal;
        m_server->setConfiguration(configuration);
    }

    QObject *SynthesisEnvironmentWelcomeWizardPageHelper::localSettings() const {
        return m_localSettings;
    }

    QAbstractItemModel *SynthesisEnvironmentWelcomeWizardPageHelper::serviceModel() const {
        return m_serviceModel;
    }

    QUuid SynthesisEnvironmentWelcomeWizardPageHelper::currentServiceId() const {
        return m_currentServiceId;
    }

    void SynthesisEnvironmentWelcomeWizardPageHelper::setCurrentServiceId(const QUuid &id) {
        if (m_currentServiceId == id)
            return;
        m_currentServiceId = id;
        Q_EMIT connectionStatusChanged();
    }

    QString SynthesisEnvironmentWelcomeWizardPageHelper::errorMessage() const {
        return m_errorMessage;
    }

    QString SynthesisEnvironmentWelcomeWizardPageHelper::connectionStatus() const {
        const auto configurations = m_service->serviceConfigurations();
        for (const auto &configuration : configurations) {
            if (configuration.id() != m_currentServiceId)
                continue;
            const auto details = m_service->serviceInstanceDetails(configuration.id());
            switch (details.healthStatus()) {
                case ServiceInstanceDetails::Disabled: return tr("Service disabled");
                case ServiceInstanceDetails::Healthy: return tr("Connected");
                case ServiceInstanceDetails::Error: return details.errorMessage().isEmpty() ? tr("Connection failed") : details.errorMessage();
                default: return tr("Connecting...");
            }
        }
        return {};
    }

    bool SynthesisEnvironmentWelcomeWizardPageHelper::connectionFailed() const {
        return m_service->serviceInstanceDetails(m_currentServiceId).healthStatus() == ServiceInstanceDetails::Error;
    }

    void SynthesisEnvironmentWelcomeWizardPageHelper::enter() {
        m_active = true;
        reloadLocalSettings();
        if (!m_serviceEdited)
            reloadServiceConfigurations();
    }

    void SynthesisEnvironmentWelcomeWizardPageHelper::leave() {
        if (!m_active)
            return;
        if (m_serviceEdited)
            applyServices(m_lastValidServices);
        if (useLocal() && !m_server->running()) {
            if (m_server->canStart())
                m_server->start();
            else if (!m_server->canStop())
                m_server->restart();
        }
        m_active = false;
        m_localSettings->endSetting();
    }

    void SynthesisEnvironmentWelcomeWizardPageHelper::reloadLocalSettings() {
        QScopedValueRollback guard(m_reloadingLocal, true);
        m_localSettings->beginSetting();
    }

    void SynthesisEnvironmentWelcomeWizardPageHelper::connectServices() {
        const auto configurations = m_serviceModel->configurations();
        for (qsizetype i = 0; i < configurations.size(); ++i) {
            QStringList errors;
            if (!configurations.at(i).validate(&errors)) {
                setErrorMessage(tr("Service %L1: %2").arg(i + 1).arg(errors.join(u'\n')));
                return;
            }
        }
        const bool unchanged = m_service->serviceConfigurations() == configurations;
        if (applyServices(configurations) && unchanged)
            m_service->refreshAll();
    }

    void SynthesisEnvironmentWelcomeWizardPageHelper::applyLocalConfiguration() {
        if (!m_active || m_reloadingLocal)
            return;
        const auto previous = m_server->configuration();
        const auto current = m_localSettings->serverConfiguration();
        if (previous == current)
            return;
        const bool startupChanged = m_server->executablePath(previous) != m_server->executablePath(current)
                                    || previous.useDefaultDevice != current.useDefaultDevice
                                    || (!current.useDefaultDevice && (previous.executionProvider != current.executionProvider || previous.deviceId != current.deviceId));
        QScopedValueRollback guard(m_applyingLocal, true);
        m_server->setConfiguration(current);
        if (useLocal() && startupChanged) {
            if (m_server->executablePath().isEmpty()) {
                if (m_server->canStop())
                    m_server->stop();
            } else {
                m_server->restart();
            }
        }
    }

    void SynthesisEnvironmentWelcomeWizardPageHelper::reloadServiceConfigurations() {
        m_lastValidServices = m_service->serviceConfigurations();
        m_serviceModel->setConfigurations(m_lastValidServices);
        setErrorMessage({});
    }

    bool SynthesisEnvironmentWelcomeWizardPageHelper::applyServices(const QList<ServiceInstanceConfiguration> &configurations) {
        QString errorMessage;
        if (!m_service->replaceServiceConfigurations(configurations, &errorMessage)) {
            setErrorMessage(errorMessage);
            return false;
        }
        m_serviceEdited = false;
        setErrorMessage({});
        Q_EMIT connectionStatusChanged();
        return true;
    }

    void SynthesisEnvironmentWelcomeWizardPageHelper::setErrorMessage(const QString &message) {
        if (m_errorMessage == message)
            return;
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }

}

#include "moc_SynthesisEnvironmentWelcomeWizardPageHelper.cpp"
