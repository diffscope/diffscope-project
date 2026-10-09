// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_SYNTHSERVICE_H
#define DIFFSCOPE_SYNTH_INTERNAL_SYNTHSERVICE_H

#include <memory>

#include <QObject>

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

    class SynthService : public QObject {
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

        bool managesArchitecture(const QString &architectureId) const;
        bool refreshing() const;

    public Q_SLOTS:
        void refreshAll();

    Q_SIGNALS:
        void serviceConfigurationsChanged();
        void serviceDetailsChanged(const QUuid &serviceId);
        void managedArchitecturesChanged();
        void refreshingChanged();

    private:
        void loadSettings();
        void saveServices() const;
        void reconcileCoreMetadata();

        static SynthService *s_instance;
        SynthInterface *m_interface{};
        SynthesisTaskManager *m_taskManager{};
        Api::ApiClient *m_apiClient{};
        MetadataRefreshController *m_metadataController{};
        std::unique_ptr<CoreMetadataRegistry> m_coreRegistry;
        QList<ServiceInstanceConfiguration> m_services;
        bool m_initialized{};
        bool m_shutdown{};
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_SYNTHSERVICE_H
