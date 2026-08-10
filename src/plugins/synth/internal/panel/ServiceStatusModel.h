#ifndef DIFFSCOPE_SYNTH_SERVICESTATUSMODEL_H
#define DIFFSCOPE_SYNTH_SERVICESTATUSMODEL_H

#include <QAbstractListModel>
#include <QUuid>

#include <synth/ServiceTypes.h>

namespace Synth::Internal {

    class SynthService;

    class ServiceStatusModel final : public QAbstractListModel {
        Q_OBJECT
    public:
        enum Role {
            ServiceIdRole = Qt::UserRole + 1,
            NameRole,
            BaseUrlRole,
            HealthStatusRole,
            HealthTextRole,
            HealthIconRole,
            ErrorMessageRole,
            LastHealthCheckRole,
        };
        Q_ENUM(Role)

        explicit ServiceStatusModel(SynthService *service, QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        QHash<int, QByteArray> roleNames() const override;

    private Q_SLOTS:
        void rebuild();
        void updateService(const QUuid &serviceId);

    private:
        SynthService *m_service{};
        QList<ServiceInstanceConfiguration> m_configurations;
    };

}

#endif // DIFFSCOPE_SYNTH_SERVICESTATUSMODEL_H
