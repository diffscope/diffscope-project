#ifndef DIFFSCOPE_SYNTH_INTERNAL_SYNTHSERVICE_H
#define DIFFSCOPE_SYNTH_INTERNAL_SYNTHSERVICE_H

#include <memory>

#include <QJsonDocument>
#include <QObject>

#include <synth/ParameterConfiguration.h>
#include <synth/ServiceTypes.h>

namespace Synth {
    class SynthInterface;
    class SynthesisTaskManager;
}

namespace Synth::Internal {

    namespace Api {
        class ApiClient;
    }
    class CoreMetadataRegistry;
    class MetadataRefreshController;

    class SynthService final : public QObject {
        Q_OBJECT
        Q_PROPERTY(bool refreshing READ refreshing NOTIFY refreshingChanged)
    public:
        explicit SynthService(QObject *parent = nullptr);
        ~SynthService() override;

        static SynthService *instance();

        bool initialize(QString *errorMessage = nullptr);
        void startDelayedInitialization();
        void shutdown();

        QList<ServiceInstanceConfiguration> serviceConfigurations() const;
        ServiceInstanceDetails serviceInstanceDetails(const QUuid &serviceId) const;
        bool replaceServiceConfigurations(const QList<ServiceInstanceConfiguration> &configurations, QString *errorMessage = nullptr);

        QList<ParameterConfiguration> allParameterConfigurations() const;
        QList<ParameterConfiguration> userParameterConfigurations() const;
        bool replaceUserParameterConfigurations(const QList<ParameterConfiguration> &configurations, QString *errorMessage = nullptr);
        bool importParameterConfigurations(const QJsonDocument &document, QString *errorMessage = nullptr, QStringList *summary = nullptr);
        QJsonDocument exportParameterConfigurations() const;

        bool managesArchitecture(const QString &architectureId) const;
        bool refreshing() const;

    public Q_SLOTS:
        void refreshAll();

    Q_SIGNALS:
        void serviceConfigurationsChanged();
        void parameterConfigurationsChanged();
        void serviceDetailsChanged(const QUuid &serviceId);
        void managedArchitecturesChanged();
        void refreshingChanged();

    private:
        void loadSettings();
        void saveServices() const;
        void saveUserParameters() const;
        void reconcileCoreMetadata();

        static SynthService *s_instance;
        SynthInterface *m_interface{};
        SynthesisTaskManager *m_taskManager{};
        Api::ApiClient *m_apiClient{};
        MetadataRefreshController *m_metadataController{};
        std::unique_ptr<CoreMetadataRegistry> m_coreRegistry;
        QList<ServiceInstanceConfiguration> m_services;
        QList<ParameterConfiguration> m_userParameters;
        bool m_initialized{};
        bool m_shutdown{};
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_SYNTHSERVICE_H
