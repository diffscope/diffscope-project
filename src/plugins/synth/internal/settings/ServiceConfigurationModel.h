#ifndef DIFFSCOPE_SYNTH_SERVICECONFIGURATIONMODEL_H
#define DIFFSCOPE_SYNTH_SERVICECONFIGURATIONMODEL_H

#include <QAbstractListModel>

#include <synth/ServiceTypes.h>

namespace Synth::Internal {

    class ServiceConfigurationModel final : public QAbstractListModel {
        Q_OBJECT
    public:
        enum Role {
            IdRole = Qt::UserRole + 1,
            EnabledRole,
            NameRole,
            HostRole,
            PortRole,
            UseSslRole,
            AuthenticationEnabledRole,
            ApiKeyRole,
            EndpointPrefixRole,
            RequestTimeoutSecondsRole,
            RetryCountRole,
            TaskConcurrencyRole,
            GlobalConcurrencyRole,
            VerifySslCertificateRole,
            HealthCheckIntervalSecondsRole,
            CustomHeadersRole,
            BaseUrlRole,
        };
        Q_ENUM(Role)

        explicit ServiceConfigurationModel(QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role) override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        QHash<int, QByteArray> roleNames() const override;

        QList<ServiceInstanceConfiguration> configurations() const;
        void setConfigurations(const QList<ServiceInstanceConfiguration> &configurations);

        Q_INVOKABLE int addService();
        Q_INVOKABLE bool removeService(int row);
        Q_INVOKABLE bool moveService(int row, int offset);

    Q_SIGNALS:
        void edited();

    private:
        QList<ServiceInstanceConfiguration> m_configurations;
    };

}

#endif // DIFFSCOPE_SYNTH_SERVICECONFIGURATIONMODEL_H
