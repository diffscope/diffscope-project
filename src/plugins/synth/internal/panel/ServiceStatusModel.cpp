// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ServiceStatusModel.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include <QDateTime>
#include <QLocale>
#include <QSet>
#include <QTimer>
#include <QUrl>

#include <synth/ServiceTypes.h>
#include <synth/SynthInterface.h>
#include <synth/SynthesisTask.h>
#include <synth/SynthesisTaskManager.h>
#include <synth/internal/SynthService.h>

namespace Synth::Internal {

    class ServiceTaskModel final : public QAbstractListModel {
    public:
        enum Role {
            TaskRole = Qt::UserRole + 1,
        };

        explicit ServiceTaskModel(QObject *parent = nullptr)
            : QAbstractListModel(parent) {
        }

        int rowCount(const QModelIndex &parent = {}) const override {
            return parent.isValid() ? 0 : m_tasks.size();
        }

        QVariant data(const QModelIndex &index, int role) const override {
            if (!index.isValid() || index.row() < 0 || index.row() >= m_tasks.size() || role != TaskRole) {
                return {};
            }
            return QVariant::fromValue(m_tasks.at(index.row()));
        }

        QHash<int, QByteArray> roleNames() const override {
            return {{TaskRole, "task"}};
        }

        void setTasks(const QList<SynthesisTask *> &tasks) {
            const QSet<SynthesisTask *> desired(tasks.cbegin(), tasks.cend());
            for (qsizetype index = m_tasks.size() - 1; index >= 0; --index) {
                if (desired.contains(m_tasks.at(index))) {
                    continue;
                }
                beginRemoveRows({}, static_cast<int>(index), static_cast<int>(index));
                m_tasks.removeAt(index);
                endRemoveRows();
            }
            for (qsizetype index = 0; index < tasks.size(); ++index) {
                if (index < m_tasks.size() && m_tasks.at(index) == tasks.at(index)) {
                    continue;
                }
                const auto current = m_tasks.indexOf(tasks.at(index), index + 1);
                if (current >= 0) {
                    beginMoveRows({}, static_cast<int>(current), static_cast<int>(current), {}, static_cast<int>(index));
                    m_tasks.move(current, index);
                    endMoveRows();
                    continue;
                }
                beginInsertRows({}, static_cast<int>(index), static_cast<int>(index));
                m_tasks.insert(index, tasks.at(index));
                endInsertRows();
            }
        }

    private:
        QList<SynthesisTask *> m_tasks;
    };

    ServiceStatusModel::ServiceStatusModel(SynthService *service, QObject *parent)
        : QAbstractListModel(parent), m_service(service),
          m_taskManager(SynthInterface::instance()->taskManager()) {
        connect(m_service, &SynthService::serviceConfigurationsChanged, this, &ServiceStatusModel::rebuild);
        connect(m_service, &SynthService::serviceDetailsChanged, this, &ServiceStatusModel::updateService);
        connect(m_taskManager, &SynthesisTaskManager::taskAdded, this, &ServiceStatusModel::scheduleTaskUpdate);
        connect(m_taskManager, &SynthesisTaskManager::taskChanged, this, &ServiceStatusModel::scheduleTaskUpdate);
        connect(m_taskManager, &SynthesisTaskManager::taskRemoved, this, [this] { synchronizeTasks(); });
        rebuild();
    }

    int ServiceStatusModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_configurations.size();
    }

    QVariant ServiceStatusModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_configurations.size())
            return {};
        const auto configuration = m_configurations.at(index.row());
        switch (role) {
            case ServiceIdRole:
                return configuration.id();
            case NameRole:
                return configuration.name();
            case BaseUrlRole:
                return configuration.baseUrl().toString(QUrl::FullyEncoded);
            case RunningTaskCountRole:
                return m_runningTaskCounts.value(configuration.id());
            case QueuedTaskCountRole:
                return m_queuedTaskCounts.value(configuration.id());
            case TaskCountRole: {
                const auto taskModel = m_taskModels.value(configuration.id());
                return taskModel ? taskModel->rowCount() : 0;
            }
            case TasksRole:
                return QVariant::fromValue(static_cast<QAbstractItemModel *>(m_taskModels.value(configuration.id())));
            default:
                break;
        }
        const auto details = m_service->serviceInstanceDetails(configuration.id());
        switch (role) {
            case HealthStatusRole:
                return static_cast<int>(details.healthStatus());
            case HealthTextRole:
                switch (details.healthStatus()) {
                    case ServiceInstanceDetails::Disabled:
                        return tr("Disabled");
                    case ServiceInstanceDetails::Unknown:
                        return tr("Not checked");
                    case ServiceInstanceDetails::Checking:
                        return tr("Checking...");
                    case ServiceInstanceDetails::Healthy:
                        return tr("Connected");
                    case ServiceInstanceDetails::Error:
                        return tr("Error");
                }
                return {};
            case HealthIconRole:
                switch (details.healthStatus()) {
                    case ServiceInstanceDetails::Disabled:
                        return QStringLiteral("subtract_circle");
                    case ServiceInstanceDetails::Unknown:
                        return QStringLiteral("question_circle");
                    case ServiceInstanceDetails::Checking:
                        return QStringLiteral("arrow_sync");
                    case ServiceInstanceDetails::Healthy:
                        return QStringLiteral("checkmark_circle");
                    case ServiceInstanceDetails::Error:
                        return QStringLiteral("dismiss_circle");
                }
                return QStringLiteral("question_circle");
            case ErrorMessageRole:
                return details.errorMessage();
            case LastHealthCheckRole:
                return details.lastHealthCheck().isValid()
                           ? QLocale().toString(details.lastHealthCheck().toLocalTime(), QLocale::ShortFormat)
                           : QString{};
            default:
                return {};
        }
    }

    QHash<int, QByteArray> ServiceStatusModel::roleNames() const {
        return {
            {ServiceIdRole, "serviceId"},
            {NameRole, "name"},
            {BaseUrlRole, "baseUrl"},
            {HealthStatusRole, "healthStatus"},
            {HealthTextRole, "healthText"},
            {HealthIconRole, "healthIcon"},
            {ErrorMessageRole, "errorMessage"},
            {LastHealthCheckRole, "lastHealthCheck"},
            {RunningTaskCountRole, "runningTaskCount"},
            {QueuedTaskCountRole, "queuedTaskCount"},
            {TaskCountRole, "taskCount"},
            {TasksRole, "tasks"},
        };
    }

    void ServiceStatusModel::rebuild() {
        beginResetModel();
        m_configurations = m_service->serviceConfigurations();
        QSet<QUuid> serviceIds;
        for (const auto &configuration : std::as_const(m_configurations)) {
            serviceIds.insert(configuration.id());
            if (!m_taskModels.contains(configuration.id())) {
                m_taskModels.insert(configuration.id(), new ServiceTaskModel(this));
            }
        }
        for (auto it = m_taskModels.begin(); it != m_taskModels.end();) {
            if (serviceIds.contains(it.key())) {
                ++it;
                continue;
            }
            it.value()->deleteLater();
            it = m_taskModels.erase(it);
        }
        m_runningTaskCounts.clear();
        m_queuedTaskCounts.clear();
        synchronizeTasks(false);
        endResetModel();
    }

    void ServiceStatusModel::updateService(const QUuid &serviceId) {
        const auto found = std::ranges::find_if(m_configurations, [&serviceId](const auto &configuration) {
            return configuration.id() == serviceId;
        });
        if (found == m_configurations.cend())
            return;
        const int row = static_cast<int>(std::distance(m_configurations.begin(), found));
        const auto index = createIndex(row, 0);
        Q_EMIT dataChanged(index, index, {
                                             NameRole,
                                             BaseUrlRole,
                                             HealthStatusRole,
                                             HealthTextRole,
                                             HealthIconRole,
                                             ErrorMessageRole,
                                             LastHealthCheckRole,
                                         });
    }

    void ServiceStatusModel::scheduleTaskUpdate() {
        if (m_taskUpdatePending) {
            return;
        }
        m_taskUpdatePending = true;
        QTimer::singleShot(0, this, [this] {
            m_taskUpdatePending = false;
            synchronizeTasks();
        });
    }

    void ServiceStatusModel::synchronizeTasks(bool notify) {
        QHash<QUuid, QList<SynthesisTask *>> tasksByService;
        QHash<QUuid, int> runningTaskCounts;
        QHash<QUuid, int> queuedTaskCounts;
        for (auto task : m_taskManager->tasks()) {
            const auto serviceId = task->serviceInstanceId();
            if (!m_taskModels.contains(serviceId)) {
                continue;
            }
            switch (task->state()) {
                case SynthesisTask::Queued:
                    ++queuedTaskCounts[serviceId];
                    tasksByService[serviceId].append(task);
                    break;
                case SynthesisTask::Running:
                    ++runningTaskCounts[serviceId];
                    tasksByService[serviceId].append(task);
                    break;
                case SynthesisTask::Failed:
                    tasksByService[serviceId].append(task);
                    break;
                case SynthesisTask::Succeeded:
                case SynthesisTask::Canceled:
                    break;
            }
        }
        for (int row = 0; row < m_configurations.size(); ++row) {
            const auto serviceId = m_configurations.at(row).id();
            auto taskModel = m_taskModels.value(serviceId);
            const int previousTaskCount = taskModel->rowCount();
            taskModel->setTasks(tasksByService.value(serviceId));
            QList<int> roles;
            if (m_runningTaskCounts.value(serviceId) != runningTaskCounts.value(serviceId)) {
                m_runningTaskCounts.insert(serviceId, runningTaskCounts.value(serviceId));
                roles.append(RunningTaskCountRole);
            }
            if (m_queuedTaskCounts.value(serviceId) != queuedTaskCounts.value(serviceId)) {
                m_queuedTaskCounts.insert(serviceId, queuedTaskCounts.value(serviceId));
                roles.append(QueuedTaskCountRole);
            }
            if (previousTaskCount != taskModel->rowCount()) {
                roles.append(TaskCountRole);
            }
            if (notify && !roles.isEmpty()) {
                const auto item = createIndex(row, 0);
                Q_EMIT dataChanged(item, item, roles);
            }
        }
    }

}
