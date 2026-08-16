// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SERVICETYPES_P_H
#define DIFFSCOPE_SYNTH_SERVICETYPES_P_H

#include <QSharedData>

#include <synth/ServiceTypes.h>

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

    class ParameterMetadataData : public QSharedData {
    public:
        QString id;
        ParameterMetadata::Kind kind{ParameterMetadata::Direct};
        QStringList dependsOn;
        QJsonObject extra;
    };

    class ArchitectureMetadataData : public QSharedData {
    public:
        QString id;
        QString name;
        QString pronunciationMode;
        QString phonemeMode;
        QList<ParameterMetadata> parameters;
        QStringList audioDependencies;
        QJsonObject extra;
    };

    class SingerMetadataData : public QSharedData {
    public:
        QString id;
        QString architectureId;
        QString name;
        QString mixGroup;
        SingerMetadata::LanguageMap languages;
        QString defaultLanguage;
        QJsonValue architectureSpecificInfo;
        QJsonValue defaultExtra;
        QUrl avatarUrl;
        QUrl backgroundUrl;
        QJsonArray demos;
        QJsonObject extra;
    };

    class ServiceMetadataData : public QSharedData {
    public:
        QList<ArchitectureMetadata> architectures;
        QList<SingerMetadata> singers;
    };

    class ServiceInstanceDetailsData : public QSharedData {
    public:
        ServiceInstanceDetailsData() { configuration.setId({}); }

        ServiceInstanceConfiguration configuration;
        ServiceInstanceDetails::HealthStatus healthStatus{ServiceInstanceDetails::Unknown};
        int maximumApiVersion{};
        int selectedApiVersion{};
        QDateTime lastHealthCheck;
        QDateTime lastMetadataRefresh;
        QString errorMessage;
        bool metadataStale{};
        ServiceMetadata metadata;
    };

}

#endif // DIFFSCOPE_SYNTH_SERVICETYPES_P_H
