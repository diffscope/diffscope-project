// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_METADATAREFRESHCONTROLLER_H
#define DIFFSCOPE_SYNTH_INTERNAL_METADATAREFRESHCONTROLLER_H

#include <memory>
#include <optional>

#include <QObject>

#include <synth/ServiceTypes.h>

namespace Synth::Internal::Api {
    class ApiClient;
}

namespace Synth::Internal {

    class MetadataRefreshController final : public QObject {
        Q_OBJECT
        Q_PROPERTY(bool refreshing READ isRefreshing NOTIFY refreshingChanged)
    public:
        explicit MetadataRefreshController(Api::ApiClient *apiClient, QObject *parent = nullptr);
        ~MetadataRefreshController() override;

        void setServices(const QList<ServiceInstanceConfiguration> &services);
        QList<ServiceInstanceDetails> serviceDetails() const;
        std::optional<ServiceInstanceDetails> serviceDetails(const QUuid &serviceId) const;

        bool isRefreshing() const;
        void start();
        void stop();

    public Q_SLOTS:
        void refreshAll();

    Q_SIGNALS:
        void serviceDetailsChanged(const QUuid &serviceId);
        void metadataChanged();
        void refreshingChanged();
        void serviceBecameUnhealthy(const QString &serviceName, const QString &errorMessage);
        void metadataRefreshFailed(const QString &serviceName, const QString &errorMessage);

    private:
        class Private;
        std::unique_ptr<Private> d;
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_METADATAREFRESHCONTROLLER_H
