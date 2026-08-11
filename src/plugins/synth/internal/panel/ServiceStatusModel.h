#ifndef DIFFSCOPE_SYNTH_SERVICESTATUSMODEL_H
#define DIFFSCOPE_SYNTH_SERVICESTATUSMODEL_H

#include <QAbstractListModel>
#include <QUuid>

#include <synth/ServiceTypes.h>

namespace Synth::Internal {

    class SynthService;

}

namespace Synth {
    class SynthesisTaskManager;
}

namespace Synth::Internal {

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
            RunningTaskCountRole,
            QueuedTaskCountRole,
            TasksRole,
        };
        Q_ENUM(Role)

        explicit ServiceStatusModel(SynthService *service, QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        QHash<int, QByteArray> roleNames() const override;

    private Q_SLOTS:
        void rebuild();
        void updateService(const QUuid &serviceId);
        void updateServiceTasks(const QUuid &serviceId);

    private:
        SynthService *m_service{};
        QList<ServiceInstanceConfiguration> m_configurations;
        SynthesisTaskManager *m_taskManager{};
    };

}

#endif // DIFFSCOPE_SYNTH_SERVICESTATUSMODEL_H
