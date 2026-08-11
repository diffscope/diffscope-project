#include "SynthService.h"

#include <algorithm>
#include <utility>

#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMap>
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
        constexpr auto parametersKey = "parameters";
        constexpr auto parameterFormat = "org.diffscope.synth.parameter-config";
        constexpr int schemaVersion = 1;

        template <typename T>
        void sortParameterConfigurations(QList<T> &configurations) {
            std::sort(configurations.begin(), configurations.end(), [](const auto &left, const auto &right) {
                if (left.architectureId() != right.architectureId())
                    return left.architectureId() < right.architectureId();
                return left.id() < right.id();
            });
        }

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
                SynthService::tr("Synthesis service unavailable"),
                SynthService::tr("%1: %2").arg(serviceName, message)
            );
        });
        connect(m_metadataController, &MetadataRefreshController::metadataRefreshFailed, this, [](const QString &serviceName, const QString &message) {
            Core::CoreInterface::sendNotification(
                SVS::SVSCraft::Critical,
                SynthService::tr("Could not refresh singer metadata"),
                SynthService::tr("%1: %2").arg(serviceName, message)
            );
        });
        connect(m_interface, &SynthInterface::builtinParameterConfigurationsChanged, this, [this] {
            QSet<QString> builtinIds;
            for (const auto &configuration : m_interface->builtinParameterConfigurations())
                builtinIds.insert(configuration.id());
            const auto oldSize = m_userParameters.size();
            m_userParameters.removeIf([&builtinIds](const ParameterConfiguration &configuration) {
                return builtinIds.contains(configuration.id());
            });
            if (m_userParameters.size() != oldSize && m_initialized)
                saveUserParameters();
            Q_EMIT parameterConfigurationsChanged();
            reconcileCoreMetadata();
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
                *errorMessage = tr("The application settings service is not available.");
            qCCritical(lcSynthService) << "Initialization failed: application settings are unavailable";
            return false;
        }
        loadSettings();
        m_interface->setServiceInstances(m_services);
        m_metadataController->setServices(m_services);
        m_initialized = true;
        qCInfo(lcSynthService) << "Initialized with" << m_services.size()
                               << "DSSP service instance(s),"
                               << m_interface->builtinParameterConfigurations().size()
                               << "built-in parameter configuration(s), and"
                               << m_userParameters.size() << "user parameter configuration(s)";
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
                    *errorMessage = tr("Service %1: %2").arg(index + 1).arg(errors.join(QStringLiteral("; ")));
                return false;
            }
            if (ids.contains(configurations.at(index).id())) {
                if (errorMessage)
                    *errorMessage = tr("Service identifiers must be unique.");
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

    QList<ParameterConfiguration> SynthService::allParameterConfigurations() const {
        auto result = m_interface->builtinParameterConfigurations();
        QSet<QString> usedIds;
        for (const auto &configuration : std::as_const(result))
            usedIds.insert(configuration.id());
        for (const auto &configuration : m_userParameters) {
            if (!usedIds.contains(configuration.id())) {
                result.append(configuration);
                usedIds.insert(configuration.id());
            }
        }
        sortParameterConfigurations(result);
        return result;
    }

    QList<ParameterConfiguration> SynthService::userParameterConfigurations() const {
        return m_userParameters;
    }

    bool SynthService::replaceUserParameterConfigurations(
        const QList<ParameterConfiguration> &configurations, QString *errorMessage
    ) {
        QSet<QString> builtinIds;
        for (const auto &configuration : m_interface->builtinParameterConfigurations())
            builtinIds.insert(configuration.id());
        QSet<QString> ids;
        QList<ParameterConfiguration> accepted;
        accepted.reserve(configurations.size());
        for (const auto &configuration : configurations) {
            if (configuration.id() == QStringLiteral("pitch") || builtinIds.contains(configuration.id()))
                continue;
            QStringList errors;
            if (!configuration.validate(&errors)) {
                if (errorMessage)
                    *errorMessage = errors.join(QStringLiteral("; "));
                return false;
            }
            if (ids.contains(configuration.id())) {
                if (errorMessage)
                    *errorMessage = tr("Parameter identifiers must be unique.");
                return false;
            }
            ids.insert(configuration.id());
            accepted.append(configuration);
        }
        sortParameterConfigurations(accepted);
        if (m_userParameters == accepted)
            return true;
        m_userParameters = accepted;
        saveUserParameters();
        Q_EMIT parameterConfigurationsChanged();
        reconcileCoreMetadata();
        return true;
    }

    bool SynthService::importParameterConfigurations(const QJsonDocument &document, QString *errorMessage, QStringList *summary) {
        if (!document.isObject()) {
            if (errorMessage)
                *errorMessage = tr("The parameter configuration file must contain a JSON object.");
            return false;
        }
        const auto root = document.object();
        if (root.value(QStringLiteral("format")).toString() != QString::fromLatin1(parameterFormat) ||
            root.value(QStringLiteral("version")).toInt(-1) != schemaVersion ||
            !root.value(QStringLiteral("parameters")).isArray()) {
            if (errorMessage)
                *errorMessage = tr("The parameter configuration file has an unsupported format or version.");
            return false;
        }

        QSet<QString> builtinIds;
        for (const auto &configuration : m_interface->builtinParameterConfigurations())
            builtinIds.insert(configuration.id());
        QMap<QString, ParameterConfiguration> imported;
        int ignoredReserved{};
        int ignoredBuiltin{};
        for (const auto &item : root.value(QStringLiteral("parameters")).toArray()) {
            if (!item.isObject()) {
                if (errorMessage)
                    *errorMessage = tr("Every parameter entry must be a JSON object.");
                return false;
            }
            const auto idValue = item.toObject().value(QStringLiteral("id"));
            if (!idValue.isString()) {
                if (errorMessage)
                    *errorMessage = tr("Every parameter entry must contain a string id.");
                return false;
            }
            const auto id = idValue.toString();
            if (id == QStringLiteral("pitch")) {
                ++ignoredReserved;
                continue;
            }
            if (builtinIds.contains(id)) {
                ++ignoredBuiltin;
                continue;
            }
            ParameterConfiguration configuration;
            QString parseError;
            if (!ParameterConfiguration::fromJson(item.toObject(), &configuration, &parseError)) {
                if (errorMessage)
                    *errorMessage = tr("Invalid parameter %1: %2").arg(id, parseError);
                return false;
            }
            imported.insert(id, configuration); // Last entry in the file wins.
        }

        QMap<QString, ParameterConfiguration> merged;
        for (const auto &configuration : m_userParameters)
            merged.insert(configuration.id(), configuration);
        for (auto it = imported.cbegin(); it != imported.cend(); ++it)
            merged.insert(it.key(), it.value());
        if (!replaceUserParameterConfigurations(merged.values(), errorMessage))
            return false;
        if (summary) {
            summary->append(tr("Imported %1 parameter configuration(s).").arg(imported.size()));
            if (ignoredBuiltin)
                summary->append(tr("Ignored %1 built-in parameter configuration(s).").arg(ignoredBuiltin));
            if (ignoredReserved)
                summary->append(tr("Ignored %1 reserved pitch configuration(s).").arg(ignoredReserved));
        }
        return true;
    }

    QJsonDocument SynthService::exportParameterConfigurations() const {
        QJsonArray parameters;
        for (const auto &configuration : allParameterConfigurations())
            parameters.append(configuration.toJson());
        return QJsonDocument(QJsonObject{
            {QStringLiteral("format"), QString::fromLatin1(parameterFormat)},
            {QStringLiteral("version"), schemaVersion},
            {QStringLiteral("parameters"), parameters},
        });
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

        const auto parameterDocument = parseStoredDocument(settings->value(QString::fromLatin1(parametersKey)));
        if (parameterDocument.isObject()) {
            const auto root = parameterDocument.object();
            if (root.value(QStringLiteral("version")).toInt(-1) == schemaVersion &&
                root.value(QStringLiteral("parameters")).isArray()) {
                QMap<QString, ParameterConfiguration> loaded;
                const auto builtinConfigurations = m_interface->builtinParameterConfigurations();
                QSet<QString> builtinIds;
                for (const auto &configuration : builtinConfigurations)
                    builtinIds.insert(configuration.id());
                for (const auto &item : root.value(QStringLiteral("parameters")).toArray()) {
                    if (!item.isObject())
                        continue;
                    ParameterConfiguration configuration;
                    QString error;
                    if (!ParameterConfiguration::fromJson(item.toObject(), &configuration, &error) ||
                        configuration.id() == QStringLiteral("pitch") ||
                        builtinIds.contains(configuration.id())) {
                        qCWarning(lcSynthService) << "Ignoring invalid persisted parameter" << error;
                        continue;
                    }
                    loaded.insert(configuration.id(), configuration);
                }
                m_userParameters = loaded.values();
                sortParameterConfigurations(m_userParameters);
            }
        }
        settings->endGroup();
        // Persist the generated UUID immediately so the default service has a stable identity
        // even when the user never opens the settings page.
        if (persistDefaultService)
            saveServices();
        qCInfo(lcSynthService) << "Loaded" << m_services.size()
                               << "service configuration(s) and" << m_userParameters.size()
                               << "user parameter configuration(s) from settings";
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

    void SynthService::saveUserParameters() const {
        QJsonArray parameters;
        for (const auto &configuration : m_userParameters)
            parameters.append(configuration.toJson());
        const QJsonDocument document(QJsonObject{
            {QStringLiteral("version"), schemaVersion},
            {QStringLiteral("parameters"), parameters},
        });
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QString::fromLatin1(settingsGroup));
        settings->setValue(QString::fromLatin1(parametersKey), document.toJson(QJsonDocument::Compact));
        settings->endGroup();
    }

    void SynthService::reconcileCoreMetadata() {
        if (!m_initialized || m_shutdown)
            return;
        qCDebug(lcSynthService) << "Reconciling Core singer metadata from"
                                << m_metadataController->serviceDetails().size()
                                << "service cache(s)";
        m_coreRegistry->reconcile(m_services, m_metadataController->serviceDetails(), allParameterConfigurations());
    }

}
