#ifndef DIFFSCOPE_SYNTH_SERVICETYPES_H
#define DIFFSCOPE_SYNTH_SERVICETYPES_H

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QSharedDataPointer>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUuid>

#include <synth/synthglobal.h>

namespace Synth {

    class ServiceInstanceConfigurationData;

    class SYNTH_EXPORT ServiceInstanceConfiguration {
        Q_GADGET
        Q_PROPERTY(QUuid id READ id WRITE setId)
        Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(QString host READ host WRITE setHost)
        Q_PROPERTY(int port READ port WRITE setPort)
        Q_PROPERTY(bool useSsl READ useSsl WRITE setUseSsl)
        Q_PROPERTY(bool authenticationEnabled READ authenticationEnabled WRITE setAuthenticationEnabled)
        Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey)
        Q_PROPERTY(QString endpointPrefix READ endpointPrefix WRITE setEndpointPrefix)
        Q_PROPERTY(int requestTimeoutSeconds READ requestTimeoutSeconds WRITE setRequestTimeoutSeconds)
        Q_PROPERTY(int retryCount READ retryCount WRITE setRetryCount)
        Q_PROPERTY(int taskConcurrency READ taskConcurrency WRITE setTaskConcurrency)
        Q_PROPERTY(int globalConcurrency READ globalConcurrency WRITE setGlobalConcurrency)
        Q_PROPERTY(bool verifySslCertificate READ verifySslCertificate WRITE setVerifySslCertificate)
        Q_PROPERTY(int healthCheckIntervalSeconds READ healthCheckIntervalSeconds WRITE setHealthCheckIntervalSeconds)
        Q_PROPERTY(QString customHeaders READ customHeaders WRITE setCustomHeaders)
        Q_PROPERTY(QUrl baseUrl READ baseUrl)

    public:
        ServiceInstanceConfiguration();
        ServiceInstanceConfiguration(const ServiceInstanceConfiguration &other);
        ServiceInstanceConfiguration(ServiceInstanceConfiguration &&other) noexcept;
        ServiceInstanceConfiguration &operator=(const ServiceInstanceConfiguration &other);
        ServiceInstanceConfiguration &operator=(ServiceInstanceConfiguration &&other) noexcept;
        ~ServiceInstanceConfiguration();

        QUuid id() const;
        void setId(const QUuid &id);
        bool isEnabled() const;
        void setEnabled(bool enabled);
        QString name() const;
        void setName(const QString &name);
        QString host() const;
        void setHost(const QString &host);
        int port() const;
        void setPort(int port);
        bool useSsl() const;
        void setUseSsl(bool useSsl);
        bool authenticationEnabled() const;
        void setAuthenticationEnabled(bool enabled);
        QString apiKey() const;
        void setApiKey(const QString &apiKey);
        QString endpointPrefix() const;
        void setEndpointPrefix(const QString &prefix);
        int requestTimeoutSeconds() const;
        void setRequestTimeoutSeconds(int seconds);
        int retryCount() const;
        void setRetryCount(int count);
        int taskConcurrency() const;
        void setTaskConcurrency(int count);
        int globalConcurrency() const;
        void setGlobalConcurrency(int count);
        bool verifySslCertificate() const;
        void setVerifySslCertificate(bool verify);
        int healthCheckIntervalSeconds() const;
        void setHealthCheckIntervalSeconds(int seconds);
        QString customHeaders() const;
        void setCustomHeaders(const QString &headers);

        QUrl baseUrl() const;
        QMap<QString, QString> parsedCustomHeaders() const;
        bool validate(QStringList *errors = nullptr) const;

        QJsonObject toJson() const;
        static bool fromJson(const QJsonObject &object, ServiceInstanceConfiguration *result,
                             QString *errorMessage = nullptr);
        static ServiceInstanceConfiguration defaultLocal();

        bool operator==(const ServiceInstanceConfiguration &other) const;
        bool operator!=(const ServiceInstanceConfiguration &other) const;

    private:
        QSharedDataPointer<ServiceInstanceConfigurationData> d;
    };

    class ParameterMetadataData;

    class SYNTH_EXPORT ParameterMetadata {
        Q_GADGET
        Q_PROPERTY(QString id READ id WRITE setId)
        Q_PROPERTY(Kind kind READ kind WRITE setKind)
        Q_PROPERTY(QStringList dependsOn READ dependsOn WRITE setDependsOn)
        Q_PROPERTY(QJsonObject extra READ extra WRITE setExtra)

    public:
        enum Kind {
            Direct,
            Indirect,
        };
        Q_ENUM(Kind)

        ParameterMetadata();
        ParameterMetadata(const ParameterMetadata &other);
        ParameterMetadata(ParameterMetadata &&other) noexcept;
        ParameterMetadata &operator=(const ParameterMetadata &other);
        ParameterMetadata &operator=(ParameterMetadata &&other) noexcept;
        ~ParameterMetadata();

        QString id() const;
        void setId(const QString &id);
        Kind kind() const;
        void setKind(Kind kind);
        QStringList dependsOn() const;
        void setDependsOn(const QStringList &dependsOn);
        QJsonObject extra() const;
        void setExtra(const QJsonObject &extra);

        QJsonObject toJson() const;
        static bool fromJson(const QJsonObject &object, ParameterMetadata *result,
                             QString *errorMessage = nullptr);
        bool operator==(const ParameterMetadata &other) const;
        bool operator!=(const ParameterMetadata &other) const;

    private:
        QSharedDataPointer<ParameterMetadataData> d;
    };

    class ArchitectureMetadataData;

    class SYNTH_EXPORT ArchitectureMetadata {
        Q_GADGET
        Q_PROPERTY(QString id READ id WRITE setId)
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(QString pronunciationMode READ pronunciationMode WRITE setPronunciationMode)
        Q_PROPERTY(QString phonemeMode READ phonemeMode WRITE setPhonemeMode)
        Q_PROPERTY(QList<Synth::ParameterMetadata> parameters READ parameters WRITE setParameters)
        Q_PROPERTY(QStringList audioDependencies READ audioDependencies WRITE setAudioDependencies)
        Q_PROPERTY(QJsonObject extra READ extra WRITE setExtra)

    public:
        ArchitectureMetadata();
        ArchitectureMetadata(const ArchitectureMetadata &other);
        ArchitectureMetadata(ArchitectureMetadata &&other) noexcept;
        ArchitectureMetadata &operator=(const ArchitectureMetadata &other);
        ArchitectureMetadata &operator=(ArchitectureMetadata &&other) noexcept;
        ~ArchitectureMetadata();

        QString id() const;
        void setId(const QString &id);
        QString name() const;
        void setName(const QString &name);
        QString pronunciationMode() const;
        void setPronunciationMode(const QString &mode);
        QString phonemeMode() const;
        void setPhonemeMode(const QString &mode);
        QList<ParameterMetadata> parameters() const;
        void setParameters(const QList<ParameterMetadata> &parameters);
        QStringList audioDependencies() const;
        void setAudioDependencies(const QStringList &dependencies);
        QJsonObject extra() const;
        void setExtra(const QJsonObject &extra);

        QJsonObject toJson() const;
        static bool fromJson(const QJsonObject &object, ArchitectureMetadata *result,
                             QString *errorMessage = nullptr);
        bool operator==(const ArchitectureMetadata &other) const;
        bool operator!=(const ArchitectureMetadata &other) const;

    private:
        QSharedDataPointer<ArchitectureMetadataData> d;
    };

    struct SYNTH_EXPORT SingerLanguageMetadata {
        Q_GADGET
        Q_PROPERTY(QString name MEMBER name)
        Q_PROPERTY(QString defaultLyric MEMBER defaultLyric)

    public:
        QString name;
        QString defaultLyric;

        QJsonObject toJson() const;
        static bool fromJson(const QJsonObject &object, SingerLanguageMetadata *result,
                             QString *errorMessage = nullptr);
        bool operator==(const SingerLanguageMetadata &) const = default;
        bool operator!=(const SingerLanguageMetadata &) const = default;
    };

    class SingerMetadataData;

    class SYNTH_EXPORT SingerMetadata {
        Q_GADGET
        Q_PROPERTY(QString id READ id WRITE setId)
        Q_PROPERTY(QString architectureId READ architectureId WRITE setArchitectureId)
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(QString mixGroup READ mixGroup WRITE setMixGroup)
        Q_PROPERTY(LanguageMap languages READ languages WRITE setLanguages)
        Q_PROPERTY(QString defaultLanguage READ defaultLanguage WRITE setDefaultLanguage)
        Q_PROPERTY(QJsonValue architectureSpecificInfo READ architectureSpecificInfo WRITE setArchitectureSpecificInfo)
        Q_PROPERTY(QJsonValue defaultExtra READ defaultExtra WRITE setDefaultExtra)
        Q_PROPERTY(QUrl avatarUrl READ avatarUrl WRITE setAvatarUrl)
        Q_PROPERTY(QUrl backgroundUrl READ backgroundUrl WRITE setBackgroundUrl)
        Q_PROPERTY(QJsonArray demos READ demos WRITE setDemos)
        Q_PROPERTY(QJsonObject extra READ extra WRITE setExtra)

    public:
        using LanguageMap = QMap<QString, SingerLanguageMetadata>;

        SingerMetadata();
        SingerMetadata(const SingerMetadata &other);
        SingerMetadata(SingerMetadata &&other) noexcept;
        SingerMetadata &operator=(const SingerMetadata &other);
        SingerMetadata &operator=(SingerMetadata &&other) noexcept;
        ~SingerMetadata();

        QString id() const;
        void setId(const QString &id);
        QString architectureId() const;
        void setArchitectureId(const QString &architectureId);
        QString name() const;
        void setName(const QString &name);
        QString mixGroup() const;
        void setMixGroup(const QString &mixGroup);
        LanguageMap languages() const;
        void setLanguages(const LanguageMap &languages);
        QString defaultLanguage() const;
        void setDefaultLanguage(const QString &language);
        QJsonValue architectureSpecificInfo() const;
        void setArchitectureSpecificInfo(const QJsonValue &info);
        QJsonValue defaultExtra() const;
        void setDefaultExtra(const QJsonValue &extra);
        QUrl avatarUrl() const;
        void setAvatarUrl(const QUrl &url);
        QUrl backgroundUrl() const;
        void setBackgroundUrl(const QUrl &url);
        QJsonArray demos() const;
        void setDemos(const QJsonArray &demos);
        QJsonObject extra() const;
        void setExtra(const QJsonObject &extra);

        QJsonObject toJson() const;
        static bool fromJson(const QJsonObject &object, SingerMetadata *result,
                             QString *errorMessage = nullptr);
        bool operator==(const SingerMetadata &other) const;
        bool operator!=(const SingerMetadata &other) const;

    private:
        QSharedDataPointer<SingerMetadataData> d;
    };

    class ServiceMetadataData;

    class SYNTH_EXPORT ServiceMetadata {
        Q_GADGET
        Q_PROPERTY(QList<Synth::ArchitectureMetadata> architectures READ architectures WRITE setArchitectures)
        Q_PROPERTY(QList<Synth::SingerMetadata> singers READ singers WRITE setSingers)

    public:
        ServiceMetadata();
        ServiceMetadata(const ServiceMetadata &other);
        ServiceMetadata(ServiceMetadata &&other) noexcept;
        ServiceMetadata &operator=(const ServiceMetadata &other);
        ServiceMetadata &operator=(ServiceMetadata &&other) noexcept;
        ~ServiceMetadata();

        QList<ArchitectureMetadata> architectures() const;
        void setArchitectures(const QList<ArchitectureMetadata> &architectures);
        QList<SingerMetadata> singers() const;
        void setSingers(const QList<SingerMetadata> &singers);

        QJsonObject toJson() const;
        static bool fromJson(const QJsonObject &object, ServiceMetadata *result,
                             QString *errorMessage = nullptr);
        bool operator==(const ServiceMetadata &other) const;
        bool operator!=(const ServiceMetadata &other) const;

    private:
        QSharedDataPointer<ServiceMetadataData> d;
    };

    class ServiceInstanceDetailsData;

    class SYNTH_EXPORT ServiceInstanceDetails {
        Q_GADGET
        Q_PROPERTY(Synth::ServiceInstanceConfiguration configuration READ configuration WRITE setConfiguration)
        Q_PROPERTY(HealthStatus healthStatus READ healthStatus WRITE setHealthStatus)
        Q_PROPERTY(int maximumApiVersion READ maximumApiVersion WRITE setMaximumApiVersion)
        Q_PROPERTY(int selectedApiVersion READ selectedApiVersion WRITE setSelectedApiVersion)
        Q_PROPERTY(QDateTime lastHealthCheck READ lastHealthCheck WRITE setLastHealthCheck)
        Q_PROPERTY(QDateTime lastMetadataRefresh READ lastMetadataRefresh WRITE setLastMetadataRefresh)
        Q_PROPERTY(QString errorMessage READ errorMessage WRITE setErrorMessage)
        Q_PROPERTY(bool metadataStale READ metadataStale WRITE setMetadataStale)
        Q_PROPERTY(Synth::ServiceMetadata metadata READ metadata WRITE setMetadata)

    public:
        enum HealthStatus {
            Disabled,
            Unknown,
            Checking,
            Healthy,
            Error,
        };
        Q_ENUM(HealthStatus)

        ServiceInstanceDetails();
        ServiceInstanceDetails(const ServiceInstanceDetails &other);
        ServiceInstanceDetails(ServiceInstanceDetails &&other) noexcept;
        ServiceInstanceDetails &operator=(const ServiceInstanceDetails &other);
        ServiceInstanceDetails &operator=(ServiceInstanceDetails &&other) noexcept;
        ~ServiceInstanceDetails();

        ServiceInstanceConfiguration configuration() const;
        void setConfiguration(const ServiceInstanceConfiguration &configuration);
        HealthStatus healthStatus() const;
        void setHealthStatus(HealthStatus status);
        int maximumApiVersion() const;
        void setMaximumApiVersion(int version);
        int selectedApiVersion() const;
        void setSelectedApiVersion(int version);
        QDateTime lastHealthCheck() const;
        void setLastHealthCheck(const QDateTime &dateTime);
        QDateTime lastMetadataRefresh() const;
        void setLastMetadataRefresh(const QDateTime &dateTime);
        QString errorMessage() const;
        void setErrorMessage(const QString &message);
        bool metadataStale() const;
        void setMetadataStale(bool stale);
        ServiceMetadata metadata() const;
        void setMetadata(const ServiceMetadata &metadata);

        QJsonObject toJson() const;
        static bool fromJson(const QJsonObject &object, ServiceInstanceDetails *result,
                             QString *errorMessage = nullptr);
        bool operator==(const ServiceInstanceDetails &other) const;
        bool operator!=(const ServiceInstanceDetails &other) const;

    private:
        QSharedDataPointer<ServiceInstanceDetailsData> d;
    };

}

Q_DECLARE_METATYPE(Synth::ServiceInstanceConfiguration)
Q_DECLARE_METATYPE(Synth::ParameterMetadata)
Q_DECLARE_METATYPE(Synth::ArchitectureMetadata)
Q_DECLARE_METATYPE(Synth::SingerLanguageMetadata)
Q_DECLARE_METATYPE(Synth::SingerMetadata::LanguageMap)
Q_DECLARE_METATYPE(Synth::SingerMetadata)
Q_DECLARE_METATYPE(Synth::ServiceMetadata)
Q_DECLARE_METATYPE(Synth::ServiceInstanceDetails)

#endif // DIFFSCOPE_SYNTH_SERVICETYPES_H
