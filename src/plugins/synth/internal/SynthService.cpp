// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthService.h"

#include <utility>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QSet>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/CoreInterface.h>

#include <synth/SynthInterface.h>
#include <synth/SynthesisTaskManager.h>
#include <synth/internal/ApiClient.h>
#include <synth/internal/CoreMetadataRegistry.h>
#include <synth/internal/MetadataRefreshController.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcSynthService, "diffscope.synth.service")

    namespace {

        constexpr auto settingsGroup = "org.diffscope.synth";
        constexpr auto servicesKey = "services";
        constexpr int schemaVersion = 1;

        QJsonDocument parseStoredDocument(const QVariant &value) {
            if (!value.isValid())
                return {};
            QJsonParseError error;
            const auto document = QJsonDocument::fromJson(value.toByteArray(), &error);
            return error.error == QJsonParseError::NoError ? document : QJsonDocument{};
        }

    }

    SynthService *SynthService::s_instance{};

    SynthService::SynthService(QObject *parent)
        : QObject(parent),
          m_interface(new SynthInterface(this)),
          m_taskManager(new SynthesisTaskManager(this)),
          m_apiClient(new Api::ApiClient(this)),
          m_metadataController(new MetadataRefreshController(m_apiClient, this)),
          m_coreRegistry(std::make_unique<CoreMetadataRegistry>(this)) {
        Q_ASSERT(!s_instance);
        s_instance = this;
        m_interface->setTaskManager(m_taskManager);
        connect(m_coreRegistry.get(), &CoreMetadataRegistry::managedArchitecturesChanged, this, &SynthService::managedArchitecturesChanged);

        connect(m_metadataController, &MetadataRefreshController::serviceDetailsChanged, this, [this](const QUuid &serviceId) {
            const auto details = m_metadataController->serviceDetails(serviceId);
            if (details)
                m_interface->setServiceInstanceDetails(*details);
            Q_EMIT serviceDetailsChanged(serviceId);
        });
        connect(m_metadataController, &MetadataRefreshController::metadataChanged, this, &SynthService::reconcileCoreMetadata);
        connect(m_metadataController, &MetadataRefreshController::refreshingChanged, this, &SynthService::refreshingChanged);
        connect(m_metadataController, &MetadataRefreshController::serviceBecameUnhealthy, this, [](const QString &serviceName, const QString &message) {
            Core::CoreInterface::sendNotification(
                SVS::SVSCraft::Critical,
                Synth::Internal::SynthService::tr("Synthesis service unavailable"),
                Synth::Internal::SynthService::tr("%1: %2").arg(serviceName, message)
            );
        });
        connect(m_metadataController, &MetadataRefreshController::metadataRefreshFailed, this, [](const QString &serviceName, const QString &message) {
            Core::CoreInterface::sendNotification(
                SVS::SVSCraft::Critical,
                Synth::Internal::SynthService::tr("Could not refresh singer metadata"),
                Synth::Internal::SynthService::tr("%1: %2").arg(serviceName, message)
            );
        });
    }

    SynthService::~SynthService() {
        shutdown();
        if (s_instance == this)
            s_instance = nullptr;
    }

    SynthService *SynthService::instance() {
        return s_instance;
    }

    bool SynthService::initialize(QString *errorMessage) {
        if (m_initialized) {
            qCDebug(lcSynthService) << "Initialization skipped because the service is already initialized";
            return true;
        }
        qCInfo(lcSynthService) << "Initializing synthesis service state";
        if (!Core::RuntimeInterface::settings()) {
            if (errorMessage)
                *errorMessage = tr("The application settings service is not available");
            qCCritical(lcSynthService) << "Initialization failed: application settings are unavailable";
            return false;
        }
        loadSettings();
        m_interface->setServiceInstances(m_services);
        m_metadataController->setServices(m_services);
        m_initialized = true;
        qCInfo(lcSynthService) << "Initialized with" << m_services.size() << "DSSP service instance(s)";
        return true;
    }

    void SynthService::startDelayedInitialization() {
        if (!m_initialized || m_shutdown) {
            qCDebug(lcSynthService) << "Delayed initialization skipped"
                                    << "initialized=" << m_initialized
                                    << "shutdown=" << m_shutdown;
            return;
        }
        qCInfo(lcSynthService) << "Starting DSSP health and metadata refresh services";
        m_metadataController->start();
    }

    void SynthService::shutdown() {
        if (m_shutdown)
            return;
        qCInfo(lcSynthService) << "Stopping DSSP requests and unregistering synthesis metadata";
        m_shutdown = true;
        m_metadataController->stop();
        m_taskManager->shutdown();
        m_apiClient->shutdown();
        m_coreRegistry->clear();
        m_interface->clearParameterRuntime();
        qCInfo(lcSynthService) << "Synthesis service state stopped";
    }

    QList<ServiceInstanceConfiguration> SynthService::serviceConfigurations() const {
        return m_services;
    }

    ServiceInstanceDetails SynthService::serviceInstanceDetails(const QUuid &serviceId) const {
        const auto details = m_metadataController->serviceDetails(serviceId);
        return details.value_or(ServiceInstanceDetails{});
    }

    bool SynthService::replaceServiceConfigurations(
        const QList<ServiceInstanceConfiguration> &configurations, QString *errorMessage
    ) {
        QSet<QUuid> ids;
        for (qsizetype index = 0; index < configurations.size(); ++index) {
            QStringList errors;
            if (!configurations.at(index).validate(&errors)) {
                if (errorMessage)
                    *errorMessage = tr("Service %L1: %2").arg(index + 1).arg(errors.join(QStringLiteral("; ")));
                return false;
            }
            if (ids.contains(configurations.at(index).id())) {
                if (errorMessage)
                    *errorMessage = tr("Service identifiers must be unique");
                return false;
            }
            ids.insert(configurations.at(index).id());
        }
        if (m_services == configurations) {
            qCDebug(lcSynthService) << "Service configuration update contains no changes";
            return true;
        }
        qCInfo(lcSynthService) << "Applying" << configurations.size()
                               << "DSSP service configuration(s)";
        m_services = configurations;
        saveServices();
        m_interface->setServiceInstances(m_services);
        m_metadataController->setServices(m_services);
        Q_EMIT serviceConfigurationsChanged();
        if (m_initialized && !m_shutdown)
            refreshAll();
        return true;
    }

    bool SynthService::managesArchitecture(const QString &architectureId) const {
        return m_coreRegistry && m_coreRegistry->managesArchitecture(architectureId);
    }

    bool SynthService::refreshing() const {
        return m_metadataController->isRefreshing();
    }

    void SynthService::refreshAll() {
        if (m_shutdown) {
            qCDebug(lcSynthService) << "Refresh ignored during shutdown";
            return;
        }
        qCInfo(lcSynthService) << "Refreshing health and metadata for all DSSP services";
        m_metadataController->refreshAll();
    }

    void SynthService::loadSettings() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QString::fromLatin1(settingsGroup));

        bool persistDefaultService = false;
        const bool hasServices = settings->contains(QString::fromLatin1(servicesKey));
        const auto servicesDocument = parseStoredDocument(settings->value(QString::fromLatin1(servicesKey)));
        bool servicesValid = hasServices && servicesDocument.isObject();
        if (servicesValid) {
            const auto root = servicesDocument.object();
            servicesValid = root.value(QStringLiteral("version")).toInt(-1) == schemaVersion &&
                            root.value(QStringLiteral("services")).isArray();
            QSet<QUuid> ids;
            QList<ServiceInstanceConfiguration> loaded;
            if (servicesValid) {
                for (const auto &item : root.value(QStringLiteral("services")).toArray()) {
                    ServiceInstanceConfiguration configuration;
                    QString error;
                    if (!item.isObject() ||
                        !ServiceInstanceConfiguration::fromJson(item.toObject(), &configuration, &error) ||
                        ids.contains(configuration.id())) {
                        qCWarning(lcSynthService) << "Ignoring invalid persisted service settings" << error;
                        servicesValid = false;
                        break;
                    }
                    ids.insert(configuration.id());
                    loaded.append(configuration);
                }
            }
            if (servicesValid)
                m_services = loaded;
        }
        if (!servicesValid) {
            m_services = {ServiceInstanceConfiguration::defaultLocal()};
            persistDefaultService = true;
            if (hasServices)
                qCWarning(lcSynthService) << "Falling back to the default DSSP service";
        }

        settings->endGroup();
        // Persist the generated UUID immediately so the default service has a stable identity
        // even when the user never opens the settings page.
        if (persistDefaultService)
            saveServices();
        qCInfo(lcSynthService) << "Loaded" << m_services.size() << "service configuration(s) from settings";
    }

    void SynthService::saveServices() const {
        QJsonArray services;
        for (const auto &configuration : m_services)
            services.append(configuration.toJson());
        const QJsonDocument document(QJsonObject{
            {QStringLiteral("version"), schemaVersion},
            {QStringLiteral("services"), services},
        });
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QString::fromLatin1(settingsGroup));
        // TODO: Move API keys out of this JSON and into the platform credential manager.
        settings->setValue(QString::fromLatin1(servicesKey), document.toJson(QJsonDocument::Compact));
        settings->endGroup();
    }

    void SynthService::reconcileCoreMetadata() {
        if (!m_initialized || m_shutdown)
            return;
        qCDebug(lcSynthService) << "Reconciling Core singer metadata from"
                                << m_metadataController->serviceDetails().size()
                                << "service cache(s)";
        m_coreRegistry->reconcile(m_services, m_metadataController->serviceDetails());
    }

}
