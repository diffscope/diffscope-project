#include "ServiceTypes.h"
#include "ServiceTypes_p.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include <QJsonDocument>
#include <QRegularExpression>

namespace Synth {

    namespace {

        void setError(QString *errorMessage, const QString &message) {
            if (errorMessage)
                *errorMessage = message;
        }

        bool readRequiredString(const QJsonObject &object, const QString &key, QString *value,
                                QString *errorMessage) {
            const auto json = object.value(key);
            if (!json.isString()) {
                setError(errorMessage, QStringLiteral("%1 must be a string").arg(key));
                return false;
            }
            *value = json.toString();
            return true;
        }

        QStringList stringListFromJson(const QJsonValue &value, bool *ok) {
            QStringList result;
            if (!value.isArray()) {
                *ok = false;
                return result;
            }
            for (const auto &item : value.toArray()) {
                if (!item.isString()) {
                    *ok = false;
                    return {};
                }
                result.append(item.toString());
            }
            *ok = true;
            return result;
        }

        QJsonArray stringListToJson(const QStringList &values) {
            QJsonArray result;
            for (const auto &value : values)
                result.append(value);
            return result;
        }

        QJsonValue serializableJsonValue(const QJsonValue &value) {
            return value.isUndefined() ? QJsonValue(QJsonValue::Null) : value;
        }

        bool hasControlCharacter(const QString &value) {
            for (const auto character : value) {
                if (character.category() == QChar::Other_Control)
                    return true;
            }
            return false;
        }

    }

    ServiceInstanceConfiguration::ServiceInstanceConfiguration()
        : d(new ServiceInstanceConfigurationData) {
    }
    ServiceInstanceConfiguration::ServiceInstanceConfiguration(const ServiceInstanceConfiguration &other) = default;
    ServiceInstanceConfiguration::ServiceInstanceConfiguration(ServiceInstanceConfiguration &&other) noexcept = default;
    ServiceInstanceConfiguration &ServiceInstanceConfiguration::operator=(const ServiceInstanceConfiguration &other) = default;
    ServiceInstanceConfiguration &ServiceInstanceConfiguration::operator=(ServiceInstanceConfiguration &&other) noexcept = default;
    ServiceInstanceConfiguration::~ServiceInstanceConfiguration() = default;

    QUuid ServiceInstanceConfiguration::id() const { return d->id; }
    void ServiceInstanceConfiguration::setId(const QUuid &id) { d->id = id; }
    bool ServiceInstanceConfiguration::isEnabled() const { return d->enabled; }
    void ServiceInstanceConfiguration::setEnabled(bool enabled) { d->enabled = enabled; }
    QString ServiceInstanceConfiguration::name() const { return d->name; }
    void ServiceInstanceConfiguration::setName(const QString &name) { d->name = name; }
    QString ServiceInstanceConfiguration::host() const { return d->host; }
    void ServiceInstanceConfiguration::setHost(const QString &host) { d->host = host; }
    int ServiceInstanceConfiguration::port() const { return d->port; }
    void ServiceInstanceConfiguration::setPort(int port) { d->port = port; }
    bool ServiceInstanceConfiguration::useSsl() const { return d->useSsl; }
    void ServiceInstanceConfiguration::setUseSsl(bool useSsl) {
        if (d->useSsl == useSsl)
            return;
        if (d->port == 80 || d->port == 443)
            d->port = useSsl ? 443 : 80;
        d->useSsl = useSsl;
    }
    bool ServiceInstanceConfiguration::authenticationEnabled() const { return d->authenticationEnabled; }
    void ServiceInstanceConfiguration::setAuthenticationEnabled(bool enabled) { d->authenticationEnabled = enabled; }
    QString ServiceInstanceConfiguration::apiKey() const { return d->apiKey; }
    void ServiceInstanceConfiguration::setApiKey(const QString &apiKey) { d->apiKey = apiKey; }
    QString ServiceInstanceConfiguration::endpointPrefix() const { return d->endpointPrefix; }
    void ServiceInstanceConfiguration::setEndpointPrefix(const QString &prefix) { d->endpointPrefix = prefix; }
    int ServiceInstanceConfiguration::requestTimeoutSeconds() const { return d->requestTimeoutSeconds; }
    void ServiceInstanceConfiguration::setRequestTimeoutSeconds(int seconds) { d->requestTimeoutSeconds = seconds; }
    int ServiceInstanceConfiguration::retryCount() const { return d->retryCount; }
    void ServiceInstanceConfiguration::setRetryCount(int count) { d->retryCount = count; }
    int ServiceInstanceConfiguration::taskConcurrency() const { return d->taskConcurrency; }
    void ServiceInstanceConfiguration::setTaskConcurrency(int count) { d->taskConcurrency = count; }
    int ServiceInstanceConfiguration::globalConcurrency() const { return d->globalConcurrency; }
    void ServiceInstanceConfiguration::setGlobalConcurrency(int count) { d->globalConcurrency = count; }
    bool ServiceInstanceConfiguration::verifySslCertificate() const { return d->verifySslCertificate; }
    void ServiceInstanceConfiguration::setVerifySslCertificate(bool verify) { d->verifySslCertificate = verify; }
    int ServiceInstanceConfiguration::healthCheckIntervalSeconds() const { return d->healthCheckIntervalSeconds; }
    void ServiceInstanceConfiguration::setHealthCheckIntervalSeconds(int seconds) { d->healthCheckIntervalSeconds = seconds; }
    QString ServiceInstanceConfiguration::customHeaders() const { return d->customHeaders; }
    void ServiceInstanceConfiguration::setCustomHeaders(const QString &headers) { d->customHeaders = headers; }

    QUrl ServiceInstanceConfiguration::baseUrl() const {
        QUrl url;
        url.setScheme(d->useSsl ? QStringLiteral("https") : QStringLiteral("http"));
        url.setHost(d->host.trimmed());
        url.setPort(d->port);
        auto path = d->endpointPrefix.trimmed();
        if (!path.isEmpty() && !path.startsWith(u'/'))
            path.prepend(u'/');
        while (path.size() > 1 && path.endsWith(u'/'))
            path.chop(1);
        url.setPath(path);
        return url;
    }

    QMap<QString, QString> ServiceInstanceConfiguration::parsedCustomHeaders() const {
        // QMap's case-insensitive lookup is emulated while inserting so the final spelling and
        // value both come from the last occurrence, as required by HTTP field semantics.
        QMap<QString, QString> result;
        QMap<QString, QString> normalizedKeys;
        const auto lines = d->customHeaders.split(u'\n');
        for (auto line : lines) {
            if (line.endsWith(u'\r'))
                line.chop(1);
            if (line.trimmed().isEmpty())
                continue;
            const auto separator = line.indexOf(u':');
            if (separator <= 0)
                continue;
            const auto name = line.left(separator).trimmed();
            const auto value = line.mid(separator + 1).trimmed();
            const auto folded = name.toCaseFolded();
            const auto oldName = normalizedKeys.value(folded);
            if (!oldName.isEmpty())
                result.remove(oldName);
            normalizedKeys.insert(folded, name);
            result.insert(name, value);
        }
        return result;
    }

    bool ServiceInstanceConfiguration::validate(QStringList *errors) const {
        QStringList localErrors;
        if (d->id.isNull())
            localErrors.append(QStringLiteral("Service id must not be null"));
        if (d->name.trimmed().isEmpty() || hasControlCharacter(d->name))
            localErrors.append(QStringLiteral("Service name must not be empty or contain control characters"));
        if (d->host.trimmed().isEmpty() || d->host.contains(u'/') || d->host.contains(u'?') ||
            d->host.contains(u'#') || hasControlCharacter(d->host)) {
            localErrors.append(QStringLiteral("Host is invalid"));
        }
        const auto candidateUrl = baseUrl();
        if (!candidateUrl.isValid() || candidateUrl.host().isEmpty())
            localErrors.append(QStringLiteral("Host does not form a valid HTTP URL"));
        if (d->port < 1 || d->port > 65535)
            localErrors.append(QStringLiteral("Port must be between 1 and 65535"));
        if (d->authenticationEnabled && d->apiKey.trimmed().isEmpty())
            localErrors.append(QStringLiteral("An API key is required when authentication is enabled"));
        if (d->authenticationEnabled && std::any_of(d->apiKey.cbegin(), d->apiKey.cend(),
                                                     [](QChar character) { return character.isSpace(); })) {
            localErrors.append(QStringLiteral("API key must not contain whitespace"));
        }
        if (hasControlCharacter(d->apiKey))
            localErrors.append(QStringLiteral("API key must not contain control characters"));
        if (d->requestTimeoutSeconds <= 0)
            localErrors.append(QStringLiteral("Request timeout must be positive"));
        if (d->retryCount < 0)
            localErrors.append(QStringLiteral("Retry count must not be negative"));
        if (d->taskConcurrency <= 0 || d->globalConcurrency <= 0)
            localErrors.append(QStringLiteral("Concurrency limits must be positive"));
        else if (d->taskConcurrency > d->globalConcurrency)
            localErrors.append(QStringLiteral("Task concurrency must not exceed global concurrency"));
        if (d->healthCheckIntervalSeconds <= 0)
            localErrors.append(QStringLiteral("Health check interval must be positive"));

        const auto prefix = d->endpointPrefix.trimmed();
        if (!prefix.isEmpty()) {
            if (!prefix.startsWith(u'/') || prefix.contains(u'?') || prefix.contains(u'#') ||
                prefix.contains(u'\\') || hasControlCharacter(prefix)) {
                localErrors.append(QStringLiteral("Endpoint prefix must be an absolute URL path"));
            }
            const auto segments = prefix.split(u'/', Qt::SkipEmptyParts);
            if (segments.contains(QStringLiteral("..")) || segments.contains(QStringLiteral(".")))
                localErrors.append(QStringLiteral("Endpoint prefix must not contain dot segments"));
        }

        static const QRegularExpression headerNamePattern(
            QStringLiteral("^[!#$%&'*+\\-.^_`|~0-9A-Za-z]+$")
        );
        const auto lines = d->customHeaders.split(u'\n');
        for (qsizetype index = 0; index < lines.size(); ++index) {
            auto line = lines.at(index);
            if (line.endsWith(u'\r'))
                line.chop(1);
            if (line.trimmed().isEmpty())
                continue;
            const auto separator = line.indexOf(u':');
            const auto headerName = separator > 0 ? line.left(separator) : QString();
            const auto headerValue = separator > 0 ? line.mid(separator + 1) : QString();
            const auto invalidValue = std::any_of(
                headerValue.cbegin(), headerValue.cend(), [](QChar character) {
                    const auto unicode = character.unicode();
                    return (unicode < 0x20 && character != u'\t') || unicode == 0x7f;
                }
            );
            if (!headerNamePattern.match(headerName).hasMatch() || invalidValue) {
                localErrors.append(QStringLiteral("Custom header on line %1 is invalid").arg(index + 1));
            }
        }

        if (errors)
            *errors = localErrors;
        return localErrors.isEmpty();
    }

    QJsonObject ServiceInstanceConfiguration::toJson() const {
        return {
            {QStringLiteral("id"), d->id.toString(QUuid::WithoutBraces)},
            {QStringLiteral("enabled"), d->enabled},
            {QStringLiteral("name"), d->name},
            {QStringLiteral("host"), d->host},
            {QStringLiteral("port"), d->port},
            {QStringLiteral("useSsl"), d->useSsl},
            {QStringLiteral("authenticationEnabled"), d->authenticationEnabled},
            {QStringLiteral("apiKey"), d->apiKey},
            {QStringLiteral("endpointPrefix"), d->endpointPrefix},
            {QStringLiteral("requestTimeoutSeconds"), d->requestTimeoutSeconds},
            {QStringLiteral("retryCount"), d->retryCount},
            {QStringLiteral("taskConcurrency"), d->taskConcurrency},
            {QStringLiteral("globalConcurrency"), d->globalConcurrency},
            {QStringLiteral("verifySslCertificate"), d->verifySslCertificate},
            {QStringLiteral("healthCheckIntervalSeconds"), d->healthCheckIntervalSeconds},
            {QStringLiteral("customHeaders"), d->customHeaders},
        };
    }

    bool ServiceInstanceConfiguration::fromJson(const QJsonObject &object,
                                                ServiceInstanceConfiguration *result,
                                                QString *errorMessage) {
        if (!result) {
            setError(errorMessage, QStringLiteral("Result pointer must not be null"));
            return false;
        }
        ServiceInstanceConfiguration value;
        const auto idValue = object.value(QStringLiteral("id"));
        if (!idValue.isString()) {
            setError(errorMessage, QStringLiteral("id must be a string"));
            return false;
        }
        value.d->id = QUuid(idValue.toString());

        const auto readBool = [&](const QString &key, bool *target) {
            const auto json = object.value(key);
            if (!json.isBool()) {
                setError(errorMessage, QStringLiteral("%1 must be a boolean").arg(key));
                return false;
            }
            *target = json.toBool();
            return true;
        };
        const auto readInt = [&](const QString &key, int *target) {
            const auto json = object.value(key);
            if (!json.isDouble()) {
                setError(errorMessage, QStringLiteral("%1 must be an integer").arg(key));
                return false;
            }
            *target = json.toInt();
            return json.toDouble() == static_cast<double>(*target);
        };
        if (!readBool(QStringLiteral("enabled"), &value.d->enabled) ||
            !readRequiredString(object, QStringLiteral("name"), &value.d->name, errorMessage) ||
            !readRequiredString(object, QStringLiteral("host"), &value.d->host, errorMessage) ||
            !readInt(QStringLiteral("port"), &value.d->port) ||
            !readBool(QStringLiteral("useSsl"), &value.d->useSsl) ||
            !readBool(QStringLiteral("authenticationEnabled"), &value.d->authenticationEnabled) ||
            !readRequiredString(object, QStringLiteral("apiKey"), &value.d->apiKey, errorMessage) ||
            !readRequiredString(object, QStringLiteral("endpointPrefix"), &value.d->endpointPrefix, errorMessage) ||
            !readInt(QStringLiteral("requestTimeoutSeconds"), &value.d->requestTimeoutSeconds) ||
            !readInt(QStringLiteral("retryCount"), &value.d->retryCount) ||
            !readInt(QStringLiteral("taskConcurrency"), &value.d->taskConcurrency) ||
            !readInt(QStringLiteral("globalConcurrency"), &value.d->globalConcurrency) ||
            !readBool(QStringLiteral("verifySslCertificate"), &value.d->verifySslCertificate) ||
            !readInt(QStringLiteral("healthCheckIntervalSeconds"), &value.d->healthCheckIntervalSeconds) ||
            !readRequiredString(object, QStringLiteral("customHeaders"), &value.d->customHeaders, errorMessage)) {
            if (errorMessage && errorMessage->isEmpty())
                *errorMessage = QStringLiteral("Service configuration contains a value of the wrong type");
            return false;
        }
        QStringList errors;
        if (!value.validate(&errors)) {
            setError(errorMessage, errors.join(u'\n'));
            return false;
        }
        *result = std::move(value);
        return true;
    }

    ServiceInstanceConfiguration ServiceInstanceConfiguration::defaultLocal() {
        ServiceInstanceConfiguration result;
        result.d->name = QStringLiteral("Local DSSP");
        result.d->host = QStringLiteral("localhost");
        result.d->port = 13711;
        return result;
    }

    bool ServiceInstanceConfiguration::operator==(const ServiceInstanceConfiguration &other) const {
        return d.constData() == other.d.constData() || toJson() == other.toJson();
    }
    bool ServiceInstanceConfiguration::operator!=(const ServiceInstanceConfiguration &other) const {
        return !(*this == other);
    }

    ParameterMetadata::ParameterMetadata() : d(new ParameterMetadataData) {}
    ParameterMetadata::ParameterMetadata(const ParameterMetadata &other) = default;
    ParameterMetadata::ParameterMetadata(ParameterMetadata &&other) noexcept = default;
    ParameterMetadata &ParameterMetadata::operator=(const ParameterMetadata &other) = default;
    ParameterMetadata &ParameterMetadata::operator=(ParameterMetadata &&other) noexcept = default;
    ParameterMetadata::~ParameterMetadata() = default;
    QString ParameterMetadata::id() const { return d->id; }
    void ParameterMetadata::setId(const QString &id) { d->id = id; }
    ParameterMetadata::Kind ParameterMetadata::kind() const { return d->kind; }
    void ParameterMetadata::setKind(Kind kind) { d->kind = kind; }
    QStringList ParameterMetadata::dependsOn() const { return d->dependsOn; }
    void ParameterMetadata::setDependsOn(const QStringList &dependsOn) { d->dependsOn = dependsOn; }
    QJsonObject ParameterMetadata::extra() const { return d->extra; }
    void ParameterMetadata::setExtra(const QJsonObject &extra) { d->extra = extra; }
    QJsonObject ParameterMetadata::toJson() const {
        return {{QStringLiteral("id"), d->id},
                {QStringLiteral("kind"), d->kind == Direct ? QStringLiteral("direct") : QStringLiteral("indirect")},
                {QStringLiteral("dependsOn"), stringListToJson(d->dependsOn)},
                {QStringLiteral("extra"), d->extra}};
    }
    bool ParameterMetadata::fromJson(const QJsonObject &object, ParameterMetadata *result,
                                     QString *errorMessage) {
        if (!result) return false;
        ParameterMetadata value;
        QString kind;
        if (!readRequiredString(object, QStringLiteral("id"), &value.d->id, errorMessage) ||
            !readRequiredString(object, QStringLiteral("kind"), &kind, errorMessage)) return false;
        if (kind == QStringLiteral("direct")) value.d->kind = Direct;
        else if (kind == QStringLiteral("indirect")) value.d->kind = Indirect;
        else { setError(errorMessage, QStringLiteral("Unknown parameter kind")); return false; }
        bool ok{};
        value.d->dependsOn = stringListFromJson(object.value(QStringLiteral("dependsOn")), &ok);
        if (!ok || !object.value(QStringLiteral("extra")).isObject()) {
            setError(errorMessage, QStringLiteral("Invalid parameter metadata"));
            return false;
        }
        value.d->extra = object.value(QStringLiteral("extra")).toObject();
        *result = std::move(value);
        return true;
    }
    bool ParameterMetadata::operator==(const ParameterMetadata &other) const { return d.constData() == other.d.constData() || toJson() == other.toJson(); }
    bool ParameterMetadata::operator!=(const ParameterMetadata &other) const { return !(*this == other); }

    ArchitectureMetadata::ArchitectureMetadata() : d(new ArchitectureMetadataData) {}
    ArchitectureMetadata::ArchitectureMetadata(const ArchitectureMetadata &other) = default;
    ArchitectureMetadata::ArchitectureMetadata(ArchitectureMetadata &&other) noexcept = default;
    ArchitectureMetadata &ArchitectureMetadata::operator=(const ArchitectureMetadata &other) = default;
    ArchitectureMetadata &ArchitectureMetadata::operator=(ArchitectureMetadata &&other) noexcept = default;
    ArchitectureMetadata::~ArchitectureMetadata() = default;
    QString ArchitectureMetadata::id() const { return d->id; }
    void ArchitectureMetadata::setId(const QString &id) { d->id = id; }
    QString ArchitectureMetadata::name() const { return d->name; }
    void ArchitectureMetadata::setName(const QString &name) { d->name = name; }
    QString ArchitectureMetadata::pronunciationMode() const { return d->pronunciationMode; }
    void ArchitectureMetadata::setPronunciationMode(const QString &mode) { d->pronunciationMode = mode; }
    QString ArchitectureMetadata::phonemeMode() const { return d->phonemeMode; }
    void ArchitectureMetadata::setPhonemeMode(const QString &mode) { d->phonemeMode = mode; }
    QList<ParameterMetadata> ArchitectureMetadata::parameters() const { return d->parameters; }
    void ArchitectureMetadata::setParameters(const QList<ParameterMetadata> &parameters) { d->parameters = parameters; }
    QStringList ArchitectureMetadata::audioDependencies() const { return d->audioDependencies; }
    void ArchitectureMetadata::setAudioDependencies(const QStringList &dependencies) { d->audioDependencies = dependencies; }
    QJsonObject ArchitectureMetadata::extra() const { return d->extra; }
    void ArchitectureMetadata::setExtra(const QJsonObject &extra) { d->extra = extra; }
    QJsonObject ArchitectureMetadata::toJson() const {
        QJsonArray parameters;
        for (const auto &parameter : d->parameters) parameters.append(parameter.toJson());
        return {{QStringLiteral("id"), d->id}, {QStringLiteral("name"), d->name},
                {QStringLiteral("pronunciationMode"), d->pronunciationMode},
                {QStringLiteral("phonemeMode"), d->phonemeMode}, {QStringLiteral("parameters"), parameters},
                {QStringLiteral("audioDependencies"), stringListToJson(d->audioDependencies)},
                {QStringLiteral("extra"), d->extra}};
    }
    bool ArchitectureMetadata::fromJson(const QJsonObject &object, ArchitectureMetadata *result,
                                        QString *errorMessage) {
        if (!result) return false;
        ArchitectureMetadata value;
        if (!readRequiredString(object, QStringLiteral("id"), &value.d->id, errorMessage) ||
            !readRequiredString(object, QStringLiteral("name"), &value.d->name, errorMessage) ||
            !readRequiredString(object, QStringLiteral("pronunciationMode"), &value.d->pronunciationMode, errorMessage) ||
            !readRequiredString(object, QStringLiteral("phonemeMode"), &value.d->phonemeMode, errorMessage) ||
            !object.value(QStringLiteral("parameters")).isArray() ||
            !object.value(QStringLiteral("extra")).isObject()) {
            setError(errorMessage, QStringLiteral("Invalid architecture metadata"));
            return false;
        }
        for (const auto &item : object.value(QStringLiteral("parameters")).toArray()) {
            if (!item.isObject()) return false;
            ParameterMetadata parameter;
            if (!ParameterMetadata::fromJson(item.toObject(), &parameter, errorMessage)) return false;
            value.d->parameters.append(parameter);
        }
        bool ok{};
        value.d->audioDependencies = stringListFromJson(object.value(QStringLiteral("audioDependencies")), &ok);
        if (!ok) { setError(errorMessage, QStringLiteral("audioDependencies must be an array of strings")); return false; }
        value.d->extra = object.value(QStringLiteral("extra")).toObject();
        *result = std::move(value);
        return true;
    }
    bool ArchitectureMetadata::operator==(const ArchitectureMetadata &other) const { return d.constData() == other.d.constData() || toJson() == other.toJson(); }
    bool ArchitectureMetadata::operator!=(const ArchitectureMetadata &other) const { return !(*this == other); }

    QJsonObject SingerLanguageMetadata::toJson() const {
        return {
            {QStringLiteral("name"), name},
            {QStringLiteral("defaultLyric"), defaultLyric},
        };
    }

    bool SingerLanguageMetadata::fromJson(const QJsonObject &object,
                                          SingerLanguageMetadata *result,
                                          QString *errorMessage) {
        if (!result)
            return false;
        SingerLanguageMetadata value;
        if (!readRequiredString(object, QStringLiteral("name"), &value.name, errorMessage) ||
            !readRequiredString(object, QStringLiteral("defaultLyric"), &value.defaultLyric, errorMessage)) {
            return false;
        }
        *result = std::move(value);
        return true;
    }

    SingerMetadata::SingerMetadata() : d(new SingerMetadataData) {}
    SingerMetadata::SingerMetadata(const SingerMetadata &other) = default;
    SingerMetadata::SingerMetadata(SingerMetadata &&other) noexcept = default;
    SingerMetadata &SingerMetadata::operator=(const SingerMetadata &other) = default;
    SingerMetadata &SingerMetadata::operator=(SingerMetadata &&other) noexcept = default;
    SingerMetadata::~SingerMetadata() = default;
    QString SingerMetadata::id() const { return d->id; }
    void SingerMetadata::setId(const QString &id) { d->id = id; }
    QString SingerMetadata::architectureId() const { return d->architectureId; }
    void SingerMetadata::setArchitectureId(const QString &id) { d->architectureId = id; }
    QString SingerMetadata::name() const { return d->name; }
    void SingerMetadata::setName(const QString &name) { d->name = name; }
    QString SingerMetadata::mixGroup() const { return d->mixGroup; }
    void SingerMetadata::setMixGroup(const QString &group) { d->mixGroup = group; }
    SingerMetadata::LanguageMap SingerMetadata::languages() const { return d->languages; }
    void SingerMetadata::setLanguages(const LanguageMap &languages) { d->languages = languages; }
    QString SingerMetadata::defaultLanguage() const { return d->defaultLanguage; }
    void SingerMetadata::setDefaultLanguage(const QString &language) { d->defaultLanguage = language; }
    QJsonValue SingerMetadata::architectureSpecificInfo() const { return d->architectureSpecificInfo; }
    void SingerMetadata::setArchitectureSpecificInfo(const QJsonValue &info) { d->architectureSpecificInfo = info; }
    QJsonValue SingerMetadata::defaultExtra() const { return d->defaultExtra; }
    void SingerMetadata::setDefaultExtra(const QJsonValue &extra) { d->defaultExtra = extra; }
    QUrl SingerMetadata::avatarUrl() const { return d->avatarUrl; }
    void SingerMetadata::setAvatarUrl(const QUrl &url) { d->avatarUrl = url; }
    QUrl SingerMetadata::backgroundUrl() const { return d->backgroundUrl; }
    void SingerMetadata::setBackgroundUrl(const QUrl &url) { d->backgroundUrl = url; }
    QJsonArray SingerMetadata::demos() const { return d->demos; }
    void SingerMetadata::setDemos(const QJsonArray &demos) { d->demos = demos; }
    QJsonObject SingerMetadata::extra() const { return d->extra; }
    void SingerMetadata::setExtra(const QJsonObject &extra) { d->extra = extra; }
    QJsonObject SingerMetadata::toJson() const {
        QJsonObject languages;
        for (auto it = d->languages.cbegin(); it != d->languages.cend(); ++it)
            languages.insert(it.key(), it->toJson());
        return {{QStringLiteral("id"), d->id}, {QStringLiteral("architectureId"), d->architectureId},
                {QStringLiteral("name"), d->name}, {QStringLiteral("mixGroup"), d->mixGroup},
                {QStringLiteral("languages"), languages},
                {QStringLiteral("defaultLanguage"), d->defaultLanguage},
                {QStringLiteral("architectureSpecificInfo"), serializableJsonValue(d->architectureSpecificInfo)},
                {QStringLiteral("defaultExtra"), serializableJsonValue(d->defaultExtra)},
                {QStringLiteral("avatarUrl"), d->avatarUrl.toString()},
                {QStringLiteral("backgroundUrl"), d->backgroundUrl.toString()},
                {QStringLiteral("demos"), d->demos}, {QStringLiteral("extra"), d->extra}};
    }
    bool SingerMetadata::fromJson(const QJsonObject &object, SingerMetadata *result,
                                  QString *errorMessage) {
        if (!result) return false;
        SingerMetadata value;
        QString avatar, background;
        if (!readRequiredString(object, QStringLiteral("id"), &value.d->id, errorMessage) ||
            !readRequiredString(object, QStringLiteral("architectureId"), &value.d->architectureId, errorMessage) ||
            !readRequiredString(object, QStringLiteral("name"), &value.d->name, errorMessage) ||
            !readRequiredString(object, QStringLiteral("mixGroup"), &value.d->mixGroup, errorMessage) ||
            !readRequiredString(object, QStringLiteral("defaultLanguage"), &value.d->defaultLanguage, errorMessage) ||
            !readRequiredString(object, QStringLiteral("avatarUrl"), &avatar, errorMessage) ||
            !readRequiredString(object, QStringLiteral("backgroundUrl"), &background, errorMessage) ||
            !object.value(QStringLiteral("languages")).isObject() ||
            !object.value(QStringLiteral("demos")).isArray() ||
            !object.value(QStringLiteral("extra")).isObject()) {
            setError(errorMessage, QStringLiteral("Invalid singer metadata"));
            return false;
        }
        const auto languages = object.value(QStringLiteral("languages")).toObject();
        for (auto it = languages.constBegin(); it != languages.constEnd(); ++it) {
            if (!it->isObject()) {
                setError(errorMessage, QStringLiteral("Singer language '%1' must be an object").arg(it.key()));
                return false;
            }
            SingerLanguageMetadata language;
            QString languageError;
            if (!SingerLanguageMetadata::fromJson(it->toObject(), &language, &languageError)) {
                setError(errorMessage, QStringLiteral("Singer language '%1': %2").arg(it.key(), languageError));
                return false;
            }
            value.d->languages.insert(it.key(), language);
        }
        value.d->architectureSpecificInfo = object.value(QStringLiteral("architectureSpecificInfo"));
        value.d->defaultExtra = object.value(QStringLiteral("defaultExtra"));
        value.d->avatarUrl = QUrl(avatar);
        value.d->backgroundUrl = QUrl(background);
        value.d->demos = object.value(QStringLiteral("demos")).toArray();
        value.d->extra = object.value(QStringLiteral("extra")).toObject();
        *result = std::move(value);
        return true;
    }
    bool SingerMetadata::operator==(const SingerMetadata &other) const { return d.constData() == other.d.constData() || toJson() == other.toJson(); }
    bool SingerMetadata::operator!=(const SingerMetadata &other) const { return !(*this == other); }

    ServiceMetadata::ServiceMetadata() : d(new ServiceMetadataData) {}
    ServiceMetadata::ServiceMetadata(const ServiceMetadata &other) = default;
    ServiceMetadata::ServiceMetadata(ServiceMetadata &&other) noexcept = default;
    ServiceMetadata &ServiceMetadata::operator=(const ServiceMetadata &other) = default;
    ServiceMetadata &ServiceMetadata::operator=(ServiceMetadata &&other) noexcept = default;
    ServiceMetadata::~ServiceMetadata() = default;
    QList<ArchitectureMetadata> ServiceMetadata::architectures() const { return d->architectures; }
    void ServiceMetadata::setArchitectures(const QList<ArchitectureMetadata> &values) { d->architectures = values; }
    QList<SingerMetadata> ServiceMetadata::singers() const { return d->singers; }
    void ServiceMetadata::setSingers(const QList<SingerMetadata> &values) { d->singers = values; }
    QJsonObject ServiceMetadata::toJson() const {
        QJsonArray architectures, singers;
        for (const auto &item : d->architectures) architectures.append(item.toJson());
        for (const auto &item : d->singers) singers.append(item.toJson());
        return {{QStringLiteral("architectures"), architectures}, {QStringLiteral("singers"), singers}};
    }
    bool ServiceMetadata::fromJson(const QJsonObject &object, ServiceMetadata *result,
                                   QString *errorMessage) {
        if (!result || !object.value(QStringLiteral("architectures")).isArray() ||
            !object.value(QStringLiteral("singers")).isArray()) return false;
        ServiceMetadata value;
        for (const auto &item : object.value(QStringLiteral("architectures")).toArray()) {
            if (!item.isObject()) return false;
            ArchitectureMetadata metadata;
            if (!ArchitectureMetadata::fromJson(item.toObject(), &metadata, errorMessage)) return false;
            value.d->architectures.append(metadata);
        }
        for (const auto &item : object.value(QStringLiteral("singers")).toArray()) {
            if (!item.isObject()) return false;
            SingerMetadata metadata;
            if (!SingerMetadata::fromJson(item.toObject(), &metadata, errorMessage)) return false;
            value.d->singers.append(metadata);
        }
        *result = std::move(value);
        return true;
    }
    bool ServiceMetadata::operator==(const ServiceMetadata &other) const { return d.constData() == other.d.constData() || toJson() == other.toJson(); }
    bool ServiceMetadata::operator!=(const ServiceMetadata &other) const { return !(*this == other); }

    ServiceInstanceDetails::ServiceInstanceDetails() : d(new ServiceInstanceDetailsData) {}
    ServiceInstanceDetails::ServiceInstanceDetails(const ServiceInstanceDetails &other) = default;
    ServiceInstanceDetails::ServiceInstanceDetails(ServiceInstanceDetails &&other) noexcept = default;
    ServiceInstanceDetails &ServiceInstanceDetails::operator=(const ServiceInstanceDetails &other) = default;
    ServiceInstanceDetails &ServiceInstanceDetails::operator=(ServiceInstanceDetails &&other) noexcept = default;
    ServiceInstanceDetails::~ServiceInstanceDetails() = default;
    ServiceInstanceConfiguration ServiceInstanceDetails::configuration() const { return d->configuration; }
    void ServiceInstanceDetails::setConfiguration(const ServiceInstanceConfiguration &value) { d->configuration = value; }
    ServiceInstanceDetails::HealthStatus ServiceInstanceDetails::healthStatus() const { return d->healthStatus; }
    void ServiceInstanceDetails::setHealthStatus(HealthStatus value) { d->healthStatus = value; }
    int ServiceInstanceDetails::maximumApiVersion() const { return d->maximumApiVersion; }
    void ServiceInstanceDetails::setMaximumApiVersion(int value) { d->maximumApiVersion = value; }
    int ServiceInstanceDetails::selectedApiVersion() const { return d->selectedApiVersion; }
    void ServiceInstanceDetails::setSelectedApiVersion(int value) { d->selectedApiVersion = value; }
    QDateTime ServiceInstanceDetails::lastHealthCheck() const { return d->lastHealthCheck; }
    void ServiceInstanceDetails::setLastHealthCheck(const QDateTime &value) { d->lastHealthCheck = value; }
    QDateTime ServiceInstanceDetails::lastMetadataRefresh() const { return d->lastMetadataRefresh; }
    void ServiceInstanceDetails::setLastMetadataRefresh(const QDateTime &value) { d->lastMetadataRefresh = value; }
    QString ServiceInstanceDetails::errorMessage() const { return d->errorMessage; }
    void ServiceInstanceDetails::setErrorMessage(const QString &value) { d->errorMessage = value; }
    bool ServiceInstanceDetails::metadataStale() const { return d->metadataStale; }
    void ServiceInstanceDetails::setMetadataStale(bool value) { d->metadataStale = value; }
    ServiceMetadata ServiceInstanceDetails::metadata() const { return d->metadata; }
    void ServiceInstanceDetails::setMetadata(const ServiceMetadata &value) { d->metadata = value; }
    QJsonObject ServiceInstanceDetails::toJson() const {
        return {{QStringLiteral("configuration"), d->configuration.toJson()},
                {QStringLiteral("healthStatus"), static_cast<int>(d->healthStatus)},
                {QStringLiteral("maximumApiVersion"), d->maximumApiVersion},
                {QStringLiteral("selectedApiVersion"), d->selectedApiVersion},
                {QStringLiteral("lastHealthCheck"), d->lastHealthCheck.toString(Qt::ISODateWithMs)},
                {QStringLiteral("lastMetadataRefresh"), d->lastMetadataRefresh.toString(Qt::ISODateWithMs)},
                {QStringLiteral("errorMessage"), d->errorMessage},
                {QStringLiteral("metadataStale"), d->metadataStale},
                {QStringLiteral("metadata"), d->metadata.toJson()}};
    }
    bool ServiceInstanceDetails::fromJson(const QJsonObject &object, ServiceInstanceDetails *result,
                                          QString *errorMessage) {
        if (!result) {
            setError(errorMessage, QStringLiteral("result must not be null"));
            return false;
        }
        if (!object.value(QStringLiteral("configuration")).isObject()) {
            setError(errorMessage, QStringLiteral("configuration must be an object"));
            return false;
        }
        if (!object.value(QStringLiteral("metadata")).isObject()) {
            setError(errorMessage, QStringLiteral("metadata must be an object"));
            return false;
        }

        ServiceInstanceDetails value;
        QString nestedError;
        if (!ServiceInstanceConfiguration::fromJson(
                object.value(QStringLiteral("configuration")).toObject(),
                &value.d->configuration, &nestedError)) {
            setError(errorMessage, nestedError.isEmpty()
                                       ? QStringLiteral("configuration is invalid")
                                       : nestedError);
            return false;
        }
        nestedError.clear();
        if (!ServiceMetadata::fromJson(object.value(QStringLiteral("metadata")).toObject(),
                                       &value.d->metadata, &nestedError)) {
            setError(errorMessage, nestedError.isEmpty() ? QStringLiteral("metadata is invalid")
                                                         : nestedError);
            return false;
        }

        const auto readInteger = [&object, errorMessage](const QString &key, int minimum,
                                                         int maximum, int *target) {
            const auto json = object.value(key);
            if (!json.isDouble()) {
                setError(errorMessage, QStringLiteral("%1 must be an integer").arg(key));
                return false;
            }
            const double number = json.toDouble();
            if (!std::isfinite(number) || std::trunc(number) != number || number < minimum ||
                number > maximum) {
                setError(errorMessage, QStringLiteral("%1 is outside its valid range").arg(key));
                return false;
            }
            *target = static_cast<int>(number);
            return true;
        };
        const auto readDateTime = [&object, errorMessage](const QString &key, QDateTime *target) {
            const auto json = object.value(key);
            if (!json.isString()) {
                setError(errorMessage, QStringLiteral("%1 must be a string").arg(key));
                return false;
            }
            const auto text = json.toString();
            if (text.isEmpty()) {
                *target = {};
                return true;
            }
            const auto dateTime = QDateTime::fromString(text, Qt::ISODateWithMs);
            if (!dateTime.isValid()) {
                setError(errorMessage, QStringLiteral("%1 must be an ISO 8601 date and time").arg(key));
                return false;
            }
            *target = dateTime;
            return true;
        };

        int health{};
        if (!readInteger(QStringLiteral("healthStatus"), Disabled, Error, &health))
            return false;
        value.d->healthStatus = static_cast<HealthStatus>(health);
        if (!readInteger(QStringLiteral("maximumApiVersion"), 0,
                         std::numeric_limits<int>::max(), &value.d->maximumApiVersion) ||
            !readInteger(QStringLiteral("selectedApiVersion"), 0,
                         std::numeric_limits<int>::max(), &value.d->selectedApiVersion)) {
            return false;
        }
        if (value.d->selectedApiVersion > value.d->maximumApiVersion) {
            setError(errorMessage,
                     QStringLiteral("selectedApiVersion must not exceed maximumApiVersion"));
            return false;
        }
        if (!readDateTime(QStringLiteral("lastHealthCheck"), &value.d->lastHealthCheck) ||
            !readDateTime(QStringLiteral("lastMetadataRefresh"),
                          &value.d->lastMetadataRefresh)) {
            return false;
        }
        if (!readRequiredString(object, QStringLiteral("errorMessage"), &value.d->errorMessage,
                                errorMessage)) {
            return false;
        }
        const auto stale = object.value(QStringLiteral("metadataStale"));
        if (!stale.isBool()) {
            setError(errorMessage, QStringLiteral("metadataStale must be a boolean"));
            return false;
        }
        value.d->metadataStale = stale.toBool();
        *result = std::move(value);
        return true;
    }
    bool ServiceInstanceDetails::operator==(const ServiceInstanceDetails &other) const { return d.constData() == other.d.constData() || toJson() == other.toJson(); }
    bool ServiceInstanceDetails::operator!=(const ServiceInstanceDetails &other) const { return !(*this == other); }

}

#include "moc_ServiceTypes.cpp"
