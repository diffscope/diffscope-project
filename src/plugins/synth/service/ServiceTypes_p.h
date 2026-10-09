// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SERVICETYPES_P_H
#define DIFFSCOPE_SYNTH_SERVICETYPES_P_H

#include <synth/ServiceTypes.h>

#include <QSharedData>

#include <synth/internal/Dtos.h>

namespace Synth {

    class ServiceInstanceConfigurationData : public QSharedData {
    public:
        QUuid id{QUuid::createUuid()};
        bool enabled{true};
        QString name;
        QString host;
        int port{80};
        bool useSsl{false};
        bool authenticationEnabled{false};
        // TODO: Move this secret to the platform credential store when Core exposes one. The
        // current version is intentionally serialized with the rest of the QSettings payload.
        QString apiKey;
        QString endpointPrefix;
        int requestTimeoutSeconds{30};
        int retryCount{5};
        int taskConcurrency{4};
        int globalConcurrency{64};
        bool verifySslCertificate{true};
        int healthCheckIntervalSeconds{60};
        QString customHeaders;
    };

    class ArchitectureMetadataData : public QSharedData {
    public:
        QString id;
        QString name;
        QJsonObject parameters;
    };

    class SingerMetadataData : public QSharedData {
    public:
        QString id;
        QString architectureId;
        QString name;
        QString mixGroup;
        QString groupId;
        QStringList supportedParameters;
        SingerMetadata::LanguageMap languages;
        QString defaultLanguage;
        QJsonValue architectureSpecificInfo;
        QJsonValue defaultExtra;
        QUrl avatarUrl;
        QUrl backgroundUrl;
        QJsonArray demos;
    };

    class ServiceMetadataData : public QSharedData {
    public:
        QList<ArchitectureMetadata> architectures;
        QList<SingerMetadata> singers;
        QList<Internal::Api::V1::GroupMetadata> groups;
    };

    class ServiceInstanceDetailsData : public QSharedData {
    public:
        ServiceInstanceDetailsData() { configuration.setId({}); }

        ServiceInstanceConfiguration configuration;
        ServiceInstanceDetails::HealthStatus healthStatus{ServiceInstanceDetails::Unknown};
        double maximumApiVersion{};
        int selectedApiVersion{};
        QDateTime lastHealthCheck;
        QDateTime lastMetadataRefresh;
        QString errorMessage;
        bool metadataStale{};
        ServiceMetadata metadata;
    };

}

#endif // DIFFSCOPE_SYNTH_SERVICETYPES_P_H
