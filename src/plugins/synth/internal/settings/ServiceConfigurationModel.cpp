// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ServiceConfigurationModel.h"

#include <QUrl>

namespace Synth::Internal {

    ServiceConfigurationModel::ServiceConfigurationModel(QObject *parent)
        : QAbstractListModel(parent) {
    }

    int ServiceConfigurationModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_configurations.size();
    }

    QVariant ServiceConfigurationModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_configurations.size())
            return {};

        const auto &configuration = m_configurations.at(index.row());
        switch (role) {
            case IdRole:
                return configuration.id();
            case EnabledRole:
                return configuration.isEnabled();
            case NameRole:
                return configuration.name();
            case HostRole:
                return configuration.host();
            case PortRole:
                return configuration.port();
            case UseSslRole:
                return configuration.useSsl();
            case AuthenticationEnabledRole:
                return configuration.authenticationEnabled();
            case ApiKeyRole:
                return configuration.apiKey();
            case EndpointPrefixRole:
                return configuration.endpointPrefix();
            case RequestTimeoutSecondsRole:
                return configuration.requestTimeoutSeconds();
            case RetryCountRole:
                return configuration.retryCount();
            case TaskConcurrencyRole:
                return configuration.taskConcurrency();
            case GlobalConcurrencyRole:
                return configuration.globalConcurrency();
            case VerifySslCertificateRole:
                return configuration.verifySslCertificate();
            case HealthCheckIntervalSecondsRole:
                return configuration.healthCheckIntervalSeconds();
            case CustomHeadersRole:
                return configuration.customHeaders();
            case BaseUrlRole:
                return configuration.baseUrl().toString(QUrl::FullyEncoded);
            default:
                return {};
        }
    }

    bool ServiceConfigurationModel::setData(const QModelIndex &index, const QVariant &value, int role) {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_configurations.size())
            return false;

        auto &configuration = m_configurations[index.row()];
        switch (role) {
            case EnabledRole:
                configuration.setEnabled(value.toBool());
                break;
            case NameRole:
                configuration.setName(value.toString());
                break;
            case HostRole:
                configuration.setHost(value.toString());
                break;
            case PortRole:
                configuration.setPort(value.toInt());
                break;
            case UseSslRole:
                if (const bool useSsl = value.toBool(); useSsl != configuration.useSsl()) {
                    if (configuration.port() == 80 || configuration.port() == 443)
                        configuration.setPort(useSsl ? 443 : 80);
                    configuration.setUseSsl(useSsl);
                }
                break;
            case AuthenticationEnabledRole:
                configuration.setAuthenticationEnabled(value.toBool());
                break;
            case ApiKeyRole:
                configuration.setApiKey(value.toString());
                break;
            case EndpointPrefixRole:
                configuration.setEndpointPrefix(value.toString());
                break;
            case RequestTimeoutSecondsRole:
                configuration.setRequestTimeoutSeconds(value.toInt());
                break;
            case RetryCountRole:
                configuration.setRetryCount(value.toInt());
                break;
            case TaskConcurrencyRole:
                configuration.setTaskConcurrency(value.toInt());
                break;
            case GlobalConcurrencyRole:
                configuration.setGlobalConcurrency(value.toInt());
                break;
            case VerifySslCertificateRole:
                configuration.setVerifySslCertificate(value.toBool());
                break;
            case HealthCheckIntervalSecondsRole:
                configuration.setHealthCheckIntervalSeconds(value.toInt());
                break;
            case CustomHeadersRole:
                configuration.setCustomHeaders(value.toString());
                break;
            default:
                return false;
        }

        QList<int> changedRoles{role};
        if (role == UseSslRole)
            changedRoles.append(PortRole);
        if (role == HostRole || role == PortRole || role == UseSslRole || role == EndpointPrefixRole)
            changedRoles.append(BaseUrlRole);
        Q_EMIT dataChanged(index, index, changedRoles);
        Q_EMIT edited();
        return true;
    }

    Qt::ItemFlags ServiceConfigurationModel::flags(const QModelIndex &index) const {
        auto result = QAbstractListModel::flags(index);
        if (index.isValid())
            result |= Qt::ItemIsEditable;
        return result;
    }

    QHash<int, QByteArray> ServiceConfigurationModel::roleNames() const {
        return {
            {IdRole, "serviceId"},
            {EnabledRole, "enabled"},
            {NameRole, "name"},
            {HostRole, "host"},
            {PortRole, "port"},
            {UseSslRole, "useSsl"},
            {AuthenticationEnabledRole, "authenticationEnabled"},
            {ApiKeyRole, "apiKey"},
            {EndpointPrefixRole, "endpointPrefix"},
            {RequestTimeoutSecondsRole, "requestTimeoutSeconds"},
            {RetryCountRole, "retryCount"},
            {TaskConcurrencyRole, "taskConcurrency"},
            {GlobalConcurrencyRole, "globalConcurrency"},
            {VerifySslCertificateRole, "verifySslCertificate"},
            {HealthCheckIntervalSecondsRole, "healthCheckIntervalSeconds"},
            {CustomHeadersRole, "customHeaders"},
            {BaseUrlRole, "baseUrl"},
        };
    }

    QList<ServiceInstanceConfiguration> ServiceConfigurationModel::configurations() const {
        return m_configurations;
    }

    void ServiceConfigurationModel::setConfigurations(const QList<ServiceInstanceConfiguration> &configurations) {
        beginResetModel();
        m_configurations = configurations;
        endResetModel();
    }

    int ServiceConfigurationModel::addService() {
        ServiceInstanceConfiguration configuration;
        configuration.setName(tr("DSSP Service"));
        configuration.setHost(QStringLiteral("localhost"));

        const int row = m_configurations.size();
        beginInsertRows({}, row, row);
        m_configurations.append(configuration);
        endInsertRows();
        Q_EMIT edited();
        return row;
    }

    bool ServiceConfigurationModel::removeService(int row) {
        if (row < 0 || row >= m_configurations.size())
            return false;
        beginRemoveRows({}, row, row);
        m_configurations.removeAt(row);
        endRemoveRows();
        Q_EMIT edited();
        return true;
    }

    bool ServiceConfigurationModel::moveService(int row, int offset) {
        const int destination = row + offset;
        if (row < 0 || row >= m_configurations.size() || destination < 0 || destination >= m_configurations.size() || offset == 0)
            return false;

        const int destinationChild = destination > row ? destination + 1 : destination;
        if (!beginMoveRows({}, row, row, {}, destinationChild))
            return false;
        m_configurations.move(row, destination);
        endMoveRows();
        Q_EMIT edited();
        return true;
    }

}
