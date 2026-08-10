#include "ServiceStatusModel.h"

#include <algorithm>
#include <iterator>

#include <QDateTime>
#include <QLocale>
#include <QUrl>

#include <synth/ServiceTypes.h>
#include <synth/internal/SynthService.h>

namespace Synth::Internal {

    ServiceStatusModel::ServiceStatusModel(SynthService *service, QObject *parent)
        : QAbstractListModel(parent), m_service(service) {
        connect(m_service, &SynthService::serviceConfigurationsChanged,
                this, &ServiceStatusModel::rebuild);
        connect(m_service, &SynthService::serviceDetailsChanged,
                this, &ServiceStatusModel::updateService);
        rebuild();
    }

    int ServiceStatusModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_configurations.size();
    }

    QVariant ServiceStatusModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_configurations.size())
            return {};
        const auto configuration = m_configurations.at(index.row());
        const auto details = m_service->serviceInstanceDetails(configuration.id());
        switch (role) {
            case ServiceIdRole:
                return configuration.id();
            case NameRole:
                return configuration.name();
            case BaseUrlRole:
                return configuration.baseUrl().toString(QUrl::FullyEncoded);
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
        };
    }

    void ServiceStatusModel::rebuild() {
        beginResetModel();
        m_configurations = m_service->serviceConfigurations();
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

}
