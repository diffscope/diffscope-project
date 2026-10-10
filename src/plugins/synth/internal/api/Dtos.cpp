// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Dtos.h"

#include <cmath>
#include <limits>
#include <utility>

#include <QCoreApplication>

namespace Synth::Internal::Api::V1 {
namespace {

    QString translateError(const char *sourceText, const char *disambiguation = nullptr, int n = -1) {
        return QCoreApplication::translate("Synth::Internal::Api::Dtos", sourceText, disambiguation, n);
    }

    bool fail(QString *errorMessage, const QString &message) {
        if (errorMessage)
            *errorMessage = message;
        return false;
    }

    bool readObject(const QJsonValue &json, QJsonObject &object, QString *errorMessage) {
        if (!json.isObject())
            return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Expected a JSON object")));
        object = json.toObject();
        return true;
    }

    bool requiredValue(const QJsonObject &object, const char *key, QJsonValue &value,
                       QString *errorMessage) {
        const QLatin1StringView keyView(key);
        const auto it = object.constFind(keyView);
        if (it == object.constEnd())
            return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Missing required field '%1'")).arg(keyView));
        value = *it;
        return true;
    }

    bool readString(const QJsonObject &object, const char *key, QString &value,
                    QString *errorMessage) {
        QJsonValue json;
        if (!requiredValue(object, key, json, errorMessage))
            return false;
        if (!json.isString())
            return fail(errorMessage,
                        translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field '%1' must be a string")).arg(QLatin1StringView(key)));
        value = json.toString();
        return true;
    }

    bool readBool(const QJsonObject &object, const char *key, bool &value,
                  QString *errorMessage) {
        QJsonValue json;
        if (!requiredValue(object, key, json, errorMessage))
            return false;
        if (!json.isBool())
            return fail(errorMessage,
                        translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field '%1' must be a boolean")).arg(QLatin1StringView(key)));
        value = json.toBool();
        return true;
    }

    bool readNumber(const QJsonObject &object, const char *key, double &value,
                    QString *errorMessage) {
        QJsonValue json;
        if (!requiredValue(object, key, json, errorMessage))
            return false;
        if (!json.isDouble() || !std::isfinite(json.toDouble()))
            return fail(errorMessage,
                        translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field '%1' must be a finite number"))
                            .arg(QLatin1StringView(key)));
        value = json.toDouble();
        return true;
    }

    bool readInteger(const QJsonObject &object, const char *key, int &value,
                     QString *errorMessage) {
        double number{};
        if (!readNumber(object, key, number, errorMessage))
            return false;
        if (std::trunc(number) != number || number < std::numeric_limits<int>::min()
            || number > std::numeric_limits<int>::max()) {
            return fail(errorMessage,
                        translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field '%1' must be an integer")).arg(QLatin1StringView(key)));
        }
        value = static_cast<int>(number);
        return true;
    }

    bool readAny(const QJsonObject &object, const char *key, QJsonValue &value,
                 QString *errorMessage) {
        return requiredValue(object, key, value, errorMessage);
    }

    bool readStringListValue(const QJsonValue &json, QStringList &values, QString *errorMessage) {
        if (!json.isArray())
            return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Expected an array of strings")));
        QStringList result;
        const auto array = json.toArray();
        result.reserve(array.size());
        for (const auto &item : array) {
            if (!item.isString())
                return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Array item must be a string")));
            result.append(item.toString());
        }
        values = std::move(result);
        return true;
    }

    bool readStringList(const QJsonObject &object, const char *key, QStringList &values,
                        QString *errorMessage) {
        QJsonValue json;
        return requiredValue(object, key, json, errorMessage)
            && readStringListValue(json, values, errorMessage);
    }

    bool readDoubleListValue(const QJsonValue &json, QList<double> &values, QString *errorMessage) {
        if (!json.isArray())
            return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Expected an array of numbers")));
        QList<double> result;
        const auto array = json.toArray();
        result.reserve(array.size());
        for (const auto &item : array) {
            if (!item.isDouble() || !std::isfinite(item.toDouble()))
                return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Array item must be a finite number")));
            result.append(item.toDouble());
        }
        values = std::move(result);
        return true;
    }

    bool readDoubleList(const QJsonObject &object, const char *key, QList<double> &values,
                        QString *errorMessage) {
        QJsonValue json;
        return requiredValue(object, key, json, errorMessage)
            && readDoubleListValue(json, values, errorMessage);
    }

    template<typename T>
    bool readDto(const QJsonObject &object, const char *key, T &value, QString *errorMessage) {
        QJsonValue json;
        return requiredValue(object, key, json, errorMessage) && T::fromJson(json, value, errorMessage);
    }

    template<typename T>
    bool readDtoListValue(const QJsonValue &json, QList<T> &values, QString *errorMessage) {
        if (!json.isArray())
            return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Expected a JSON array")));
        QList<T> result;
        const auto array = json.toArray();
        result.reserve(array.size());
        for (qsizetype i = 0; i < array.size(); ++i) {
            T item;
            QString nestedError;
            if (!T::fromJson(array.at(i), item, &nestedError))
                return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Invalid array item %L1: %2")).arg(i + 1).arg(nestedError));
            result.append(std::move(item));
        }
        values = std::move(result);
        return true;
    }

    template<typename T>
    bool readDtoList(const QJsonObject &object, const char *key, QList<T> &values,
                     QString *errorMessage) {
        QJsonValue json;
        return requiredValue(object, key, json, errorMessage)
            && readDtoListValue(json, values, errorMessage);
    }

    template<typename T>
    bool readDtoMapValue(const QJsonValue &json, QMap<QString, T> &values, QString *errorMessage) {
        if (!json.isObject())
            return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Expected an open JSON object map")));
        QMap<QString, T> result;
        const auto object = json.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            T item;
            QString nestedError;
            if (!T::fromJson(*it, item, &nestedError)) {
                return fail(errorMessage,
                            translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Invalid map value '%1': %2")).arg(it.key(), nestedError));
            }
            result.insert(it.key(), std::move(item));
        }
        values = std::move(result);
        return true;
    }

    template<typename T>
    bool readDtoMap(const QJsonObject &object, const char *key, QMap<QString, T> &values,
                    QString *errorMessage) {
        QJsonValue json;
        return requiredValue(object, key, json, errorMessage)
            && readDtoMapValue(json, values, errorMessage);
    }

    QJsonArray stringListToJson(const QStringList &values) {
        QJsonArray array;
        for (const auto &value : values)
            array.append(value);
        return array;
    }

    QJsonArray doubleListToJson(const QList<double> &values) {
        QJsonArray array;
        for (double value : values)
            array.append(value);
        return array;
    }

    template<typename T>
    QJsonArray dtoListToJson(const QList<T> &values) {
        QJsonArray array;
        for (const auto &value : values)
            array.append(value.toJson());
        return array;
    }

    template<typename T>
    QJsonObject dtoMapToJson(const QMap<QString, T> &values) {
        QJsonObject object;
        for (auto it = values.cbegin(); it != values.cend(); ++it)
            object.insert(it.key(), it.value().toJson());
        return object;
    }

    bool validateMix(const MultiSingerContext &context, const Mix &mix,
                     QString *errorMessage) {
        const qsizetype expectedColumns = context.singers.size() - 1;
        for (qsizetype rowIndex = 0; rowIndex < mix.rows.size(); ++rowIndex) {
            const auto &row = mix.rows.at(rowIndex);
            if (row.size() != expectedColumns) {
                return fail(errorMessage,
                            translateError(QT_TRANSLATE_N_NOOP("Synth::Internal::Api::Dtos", "Mix row %L1 must contain exactly %Ln value(s)"), nullptr, static_cast<int>(expectedColumns))
                                .arg(rowIndex + 1));
            }
            double sum{};
            for (double value : row)
                sum += value;
            if (sum > 1.0 + 1e-12) {
                return fail(errorMessage,
                            translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Mix row %L1 must have a sum no greater than 1"))
                                .arg(rowIndex + 1));
            }
        }
        return true;
    }

} // namespace

QJsonValue ApplicationApiVersion::toJson() const {
    return QJsonObject{{QStringLiteral("major"), major}, {QStringLiteral("minor"), minor}};
}

bool ApplicationApiVersion::fromJson(const QJsonValue &json, ApplicationApiVersion &value, QString *errorMessage) {
    QJsonObject object;
    ApplicationApiVersion result;
    if (!readObject(json, object, errorMessage)
        || !readNumber(object, "major", result.major, errorMessage)
        || !readNumber(object, "minor", result.minor, errorMessage))
        return false;
    value = result;
    return true;
}

QJsonValue ApplicationInfo::toJson() const {
    return QJsonObject{{QStringLiteral("apiVersion"), apiVersion.toJson()}};
}

bool ApplicationInfo::fromJson(const QJsonValue &json, ApplicationInfo &value, QString *errorMessage) {
    QJsonObject object;
    ApplicationInfo result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "apiVersion", result.apiVersion, errorMessage))
        return false;
    value = result;
    return true;
}

QJsonValue ApplicationInfoResponse::toJson() const {
    return QJsonObject{{QStringLiteral("dssp"), dssp.toJson()}};
}

bool ApplicationInfoResponse::fromJson(const QJsonValue &json, ApplicationInfoResponse &value,
                                       QString *errorMessage) {
    QJsonObject object;
    ApplicationInfoResponse result;
    if (!readObject(json, object, errorMessage) || !readDto(object, "dssp", result.dssp, errorMessage))
        return false;
    value = result;
    return true;
}

QJsonValue ParameterDefinition::toJson() const {
    return QJsonObject{
        {QStringLiteral("name"), name},
        {QStringLiteral("defaultValue"), defaultValue},
        {QStringLiteral("showBaseline"), showBaseline},
        {QStringLiteral("baselineValue"), baselineValue},
        {QStringLiteral("fillMode"), fillMode},
        {QStringLiteral("fallbackOnDefaultValue"), fallbackOnDefaultValue},
        {QStringLiteral("displayValueMappingExpression"), displayValueMappingExpression},
        {QStringLiteral("displayValueInverseMappingExpression"), displayValueInverseMappingExpression},
        {QStringLiteral("displayValuePrefix"), displayValuePrefix},
        {QStringLiteral("displayValueSuffix"), displayValueSuffix},
        {QStringLiteral("displayValueDecimalPlaces"), displayValueDecimalPlaces},
    };
}

bool ParameterDefinition::fromJson(const QJsonValue &json, ParameterDefinition &value, QString *errorMessage) {
    QJsonObject object;
    ParameterDefinition result;
    if (!readObject(json, object, errorMessage)
        || !readString(object, "name", result.name, errorMessage)
        || !readNumber(object, "defaultValue", result.defaultValue, errorMessage)
        || !readBool(object, "showBaseline", result.showBaseline, errorMessage)
        || !readNumber(object, "baselineValue", result.baselineValue, errorMessage)
        || !readString(object, "fillMode", result.fillMode, errorMessage)
        || !readBool(object, "fallbackOnDefaultValue", result.fallbackOnDefaultValue, errorMessage)
        || !readAny(object, "displayValueMappingExpression", result.displayValueMappingExpression, errorMessage)
        || !readAny(object, "displayValueInverseMappingExpression", result.displayValueInverseMappingExpression, errorMessage)
        || !readString(object, "displayValuePrefix", result.displayValuePrefix, errorMessage)
        || !readString(object, "displayValueSuffix", result.displayValueSuffix, errorMessage)
        || !readInteger(object, "displayValueDecimalPlaces", result.displayValueDecimalPlaces, errorMessage))
        return false;
    if (result.defaultValue < 0 || result.defaultValue > 1 || result.baselineValue < 0 || result.baselineValue > 1
        || result.displayValueDecimalPlaces < 0
        || !QStringList{QStringLiteral("no"), QStringLiteral("top"), QStringLiteral("bottom"), QStringLiteral("baseline")}.contains(result.fillMode))
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Invalid parameter display metadata")));
    value = std::move(result);
    return true;
}

QJsonValue ParameterPipelineMetadata::toJson() const {
    QJsonObject object{{QStringLiteral("type"), type == Direct ? QStringLiteral("direct") : QStringLiteral("indirect")}};
    if (type == Indirect) {
        object.insert(QStringLiteral("dependsOn"), stringListToJson(dependsOn));
        object.insert(QStringLiteral("retakeMode"), retakeMode);
    }
    return object;
}

bool ParameterPipelineMetadata::fromJson(const QJsonValue &json, ParameterPipelineMetadata &value, QString *errorMessage) {
    QJsonObject object;
    QString type;
    ParameterPipelineMetadata result;
    if (!readObject(json, object, errorMessage) || !readString(object, "type", type, errorMessage))
        return false;
    if (type == QStringLiteral("indirect")) {
        result.type = Indirect;
        if (!readStringList(object, "dependsOn", result.dependsOn, errorMessage)
            || !readString(object, "retakeMode", result.retakeMode, errorMessage))
            return false;
        if (result.retakeMode != QStringLiteral("full") && result.retakeMode != QStringLiteral("range"))
            return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Invalid parameter retake mode")));
    } else if (type != QStringLiteral("direct")) {
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Unknown architecture parameter type '%1'")).arg(type));
    }
    value = std::move(result);
    return true;
}

QJsonValue ArchitectureMetadata::toJson() const {
    return QJsonObject{{QStringLiteral("id"), id}, {QStringLiteral("name"), name},
                       {QStringLiteral("parameters"), dtoMapToJson(parameters)}};
}

bool ArchitectureMetadata::fromJson(const QJsonValue &json, ArchitectureMetadata &value, QString *errorMessage) {
    QJsonObject object;
    ArchitectureMetadata result;
    if (!readObject(json, object, errorMessage) || !readString(object, "id", result.id, errorMessage)
        || !readString(object, "name", result.name, errorMessage)
        || !readDtoMap(object, "parameters", result.parameters, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ArchitectureMetadataList::toJson() const { return dtoListToJson(items); }

bool ArchitectureMetadataList::fromJson(const QJsonValue &json, ArchitectureMetadataList &value, QString *errorMessage) {
    return readDtoListValue(json, value.items, errorMessage);
}

QJsonValue GroupLanguageInfo::toJson() const {
    return QJsonObject{{QStringLiteral("name"), name}, {QStringLiteral("defaultLyric"), defaultLyric},
                       {QStringLiteral("pronunciationMode"), pronunciationMode}, {QStringLiteral("phonemeMode"), phonemeMode}};
}

bool GroupLanguageInfo::fromJson(const QJsonValue &json, GroupLanguageInfo &value, QString *errorMessage) {
    QJsonObject object;
    GroupLanguageInfo result;
    if (!readObject(json, object, errorMessage) || !readString(object, "name", result.name, errorMessage)
        || !readString(object, "defaultLyric", result.defaultLyric, errorMessage)
        || !readString(object, "pronunciationMode", result.pronunciationMode, errorMessage)
        || !readString(object, "phonemeMode", result.phonemeMode, errorMessage))
        return false;
    if ((result.pronunciationMode != QStringLiteral("full") && result.pronunciationMode != QStringLiteral("skip"))
        || !QStringList{QStringLiteral("full"), QStringLiteral("token_only"), QStringLiteral("skip")}.contains(result.phonemeMode))
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Invalid language conversion mode")));
    value = std::move(result);
    return true;
}

QJsonValue GroupMetadata::toJson() const {
    return QJsonObject{{QStringLiteral("id"), id}, {QStringLiteral("arch"), arch},
                       {QStringLiteral("languages"), dtoMapToJson(languages)}, {QStringLiteral("durationMode"), durationMode},
                       {QStringLiteral("parameterPipeline"), dtoMapToJson(parameterPipeline)},
                       {QStringLiteral("audioDependencies"), stringListToJson(audioDependencies)},
                       {QStringLiteral("mixable"), mixable}, {QStringLiteral("archSpecificInfo"), archSpecificInfo},
                       {QStringLiteral("defaultArchExtra"), defaultArchExtra}};
}

bool GroupMetadata::fromJson(const QJsonValue &json, GroupMetadata &value, QString *errorMessage) {
    QJsonObject object;
    GroupMetadata result;
    if (!readObject(json, object, errorMessage) || !readString(object, "id", result.id, errorMessage)
        || !readString(object, "arch", result.arch, errorMessage)
        || !readDtoMap(object, "languages", result.languages, errorMessage)
        || !readString(object, "durationMode", result.durationMode, errorMessage)
        || !readDtoMap(object, "parameterPipeline", result.parameterPipeline, errorMessage)
        || !readStringList(object, "audioDependencies", result.audioDependencies, errorMessage)
        || !readBool(object, "mixable", result.mixable, errorMessage)
        || !readAny(object, "archSpecificInfo", result.archSpecificInfo, errorMessage)
        || !readAny(object, "defaultArchExtra", result.defaultArchExtra, errorMessage))
        return false;
    if (result.durationMode != QStringLiteral("full") && result.durationMode != QStringLiteral("skip"))
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Invalid duration mode")));
    value = std::move(result);
    return true;
}

QJsonValue GroupMetadataList::toJson() const { return dtoListToJson(items); }

bool GroupMetadataList::fromJson(const QJsonValue &json, GroupMetadataList &value, QString *errorMessage) {
    return readDtoListValue(json, value.items, errorMessage);
}

QJsonValue SingerInfo::toJson() const {
    return QJsonObject{{QStringLiteral("id"), id},
                       {QStringLiteral("name"), name},
                       {QStringLiteral("arch"), arch},
                       {QStringLiteral("group"), group},
                       {QStringLiteral("defaultLanguage"), defaultLanguage},
                       {QStringLiteral("archSpecificInfo"), archSpecificInfo},
                       {QStringLiteral("defaultExtra"), defaultExtra}};
}

bool SingerInfo::fromJson(const QJsonValue &json, SingerInfo &value, QString *errorMessage) {
    QJsonObject object;
    SingerInfo result;
    if (!readObject(json, object, errorMessage) || !readString(object, "id", result.id, errorMessage)
        || !readString(object, "name", result.name, errorMessage)
        || !readString(object, "arch", result.arch, errorMessage)
        || !readString(object, "group", result.group, errorMessage)
        || !readString(object, "defaultLanguage", result.defaultLanguage, errorMessage)
        || !readAny(object, "archSpecificInfo", result.archSpecificInfo, errorMessage)
        || !readAny(object, "defaultExtra", result.defaultExtra, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue SingerInfoList::toJson() const { return dtoListToJson(items); }

bool SingerInfoList::fromJson(const QJsonValue &json, SingerInfoList &value, QString *errorMessage) {
    SingerInfoList result;
    if (!readDtoListValue(json, result.items, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

#define SYNTH_SIMPLE_STRING_DTO(Type, Member, Key)                                                                      \
    QJsonValue Type::toJson() const { return QJsonObject{{QStringLiteral(Key), Member}}; }                              \
    bool Type::fromJson(const QJsonValue &json, Type &value, QString *errorMessage) {                                   \
        QJsonObject object;                                                                                             \
        Type result;                                                                                                    \
        if (!readObject(json, object, errorMessage)                                                                     \
            || !readString(object, Key, result.Member, errorMessage))                                                   \
            return false;                                                                                               \
        value = std::move(result);                                                                                      \
        return true;                                                                                                    \
    }

SYNTH_SIMPLE_STRING_DTO(SingerAvatarResponse, avatarUrl, "avatarUrl")
SYNTH_SIMPLE_STRING_DTO(SingerBackgroundResponse, backgroundUrl, "backgroundUrl")
SYNTH_SIMPLE_STRING_DTO(EnvTagResponse, envTag, "envTag")
SYNTH_SIMPLE_STRING_DTO(AudioOutput, audioUrl, "audioUrl")

#undef SYNTH_SIMPLE_STRING_DTO

QJsonValue SingerDemoAudio::toJson() const {
    return QJsonObject{{QStringLiteral("name"), name}, {QStringLiteral("audioUrl"), audioUrl}};
}

bool SingerDemoAudio::fromJson(const QJsonValue &json, SingerDemoAudio &value,
                               QString *errorMessage) {
    QJsonObject object;
    SingerDemoAudio result;
    if (!readObject(json, object, errorMessage) || !readString(object, "name", result.name, errorMessage)
        || !readString(object, "audioUrl", result.audioUrl, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue SingerDemoAudioList::toJson() const { return dtoListToJson(items); }

bool SingerDemoAudioList::fromJson(const QJsonValue &json, SingerDemoAudioList &value,
                                   QString *errorMessage) {
    SingerDemoAudioList result;
    if (!readDtoListValue(json, result.items, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue Singer::toJson() const {
    return QJsonObject{{QStringLiteral("id"), id}, {QStringLiteral("extra"), extra}};
}

bool Singer::fromJson(const QJsonValue &json, Singer &value, QString *errorMessage) {
    QJsonObject object;
    Singer result;
    if (!readObject(json, object, errorMessage) || !readString(object, "id", result.id, errorMessage)
        || !readAny(object, "extra", result.extra, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue SingleSingerContext::toJson() const {
    return QJsonObject{{QStringLiteral("arch"), arch},
                       {QStringLiteral("archExtra"), archExtra},
                       {QStringLiteral("singer"), singer.toJson()}};
}

bool SingleSingerContext::fromJson(const QJsonValue &json, SingleSingerContext &value,
                                   QString *errorMessage) {
    QJsonObject object;
    SingleSingerContext result;
    if (!readObject(json, object, errorMessage) || !readString(object, "arch", result.arch, errorMessage)
        || !readAny(object, "archExtra", result.archExtra, errorMessage)
        || !readDto(object, "singer", result.singer, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue MultiSingerContext::toJson() const {
    return QJsonObject{{QStringLiteral("arch"), arch},
                       {QStringLiteral("archExtra"), archExtra},
                       {QStringLiteral("singers"), dtoListToJson(singers)}};
}

bool MultiSingerContext::fromJson(const QJsonValue &json, MultiSingerContext &value,
                                  QString *errorMessage) {
    QJsonObject object;
    MultiSingerContext result;
    if (!readObject(json, object, errorMessage) || !readString(object, "arch", result.arch, errorMessage)
        || !readAny(object, "archExtra", result.archExtra, errorMessage)
        || !readDtoList(object, "singers", result.singers, errorMessage))
        return false;
    if (result.singers.isEmpty())
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field 'singers' must contain at least one item")));
    value = std::move(result);
    return true;
}

QJsonValue EnvTagRequest::toJson() const {
    return QJsonObject{{QStringLiteral("context"), context.toJson()}};
}

bool EnvTagRequest::fromJson(const QJsonValue &json, EnvTagRequest &value, QString *errorMessage) {
    QJsonObject object;
    EnvTagRequest result;
    if (!readObject(json, object, errorMessage) || !readDto(object, "context", result.context, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue Lyric::toJson() const {
    return QJsonObject{{QStringLiteral("lyric"), lyric}, {QStringLiteral("language"), language}};
}

bool Lyric::fromJson(const QJsonValue &json, Lyric &value, QString *errorMessage) {
    QJsonObject object;
    Lyric result;
    if (!readObject(json, object, errorMessage) || !readString(object, "lyric", result.lyric, errorMessage)
        || !readString(object, "language", result.language, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue PronunciationInput::toJson() const {
    return QJsonObject{{QStringLiteral("notes"), dtoListToJson(notes)}};
}

bool PronunciationInput::fromJson(const QJsonValue &json, PronunciationInput &value,
                                  QString *errorMessage) {
    QJsonObject object;
    PronunciationInput result;
    if (!readObject(json, object, errorMessage) || !readDtoList(object, "notes", result.notes, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue PronunciationRequest::toJson() const {
    return QJsonObject{{QStringLiteral("context"), context.toJson()},
                       {QStringLiteral("input"), input.toJson()}};
}

bool PronunciationRequest::fromJson(const QJsonValue &json, PronunciationRequest &value,
                                    QString *errorMessage) {
    QJsonObject object;
    PronunciationRequest result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "context", result.context, errorMessage)
        || !readDto(object, "input", result.input, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue PronunciationNote::toJson() const {
    return QJsonObject{{QStringLiteral("pronunciation"), pronunciation},
                       {QStringLiteral("language"), language},
                       {QStringLiteral("syllableSliceStart"), syllableSliceStart ? QJsonValue(*syllableSliceStart) : QJsonValue(QJsonValue::Null)},
                       {QStringLiteral("syllableSliceEnd"), syllableSliceEnd ? QJsonValue(*syllableSliceEnd) : QJsonValue(QJsonValue::Null)}};
}

bool PronunciationNote::fromJson(const QJsonValue &json, PronunciationNote &value,
                                 QString *errorMessage) {
    QJsonObject object;
    PronunciationNote result;
    if (!readObject(json, object, errorMessage)
        || !readString(object, "pronunciation", result.pronunciation, errorMessage)
        || !readString(object, "language", result.language, errorMessage))
        return false;
    for (const auto &[key, target] : {std::pair{"syllableSliceStart", &result.syllableSliceStart}, std::pair{"syllableSliceEnd", &result.syllableSliceEnd}}) {
        if (object.value(QLatin1StringView(key)).isNull() || !object.contains(QLatin1StringView(key)))
            continue;
        int endpoint{};
        if (!readInteger(object, key, endpoint, errorMessage))
            return false;
        *target = endpoint;
    }
    value = std::move(result);
    return true;
}

QJsonValue PhonemeInput::toJson() const {
    return QJsonObject{{QStringLiteral("notes"), dtoListToJson(notes)}};
}

bool PhonemeInput::fromJson(const QJsonValue &json, PhonemeInput &value, QString *errorMessage) {
    QJsonObject object;
    PhonemeInput result;
    if (!readObject(json, object, errorMessage) || !readDtoList(object, "notes", result.notes, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue PhonemeRequest::toJson() const {
    return QJsonObject{{QStringLiteral("context"), context.toJson()},
                       {QStringLiteral("input"), input.toJson()}};
}

bool PhonemeRequest::fromJson(const QJsonValue &json, PhonemeRequest &value,
                              QString *errorMessage) {
    QJsonObject object;
    PhonemeRequest result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "context", result.context, errorMessage)
        || !readDto(object, "input", result.input, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue NotePosition::toJson() const {
    return QJsonObject{{QStringLiteral("gap"), gap}, {QStringLiteral("duration"), duration}};
}

bool NotePosition::fromJson(const QJsonValue &json, NotePosition &value, QString *errorMessage) {
    QJsonObject object;
    NotePosition result;
    if (!readObject(json, object, errorMessage) || !readNumber(object, "gap", result.gap, errorMessage)
        || !readNumber(object, "duration", result.duration, errorMessage))
        return false;
    if (result.gap < 0 || result.duration < 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Note position values must be non-negative")));
    value = result;
    return true;
}

QJsonValue DurationInputPhoneme::toJson() const {
    return QJsonObject{{QStringLiteral("token"), token},
                       {QStringLiteral("onset"), onset},
                       {QStringLiteral("language"), language}};
}

bool DurationInputPhoneme::fromJson(const QJsonValue &json, DurationInputPhoneme &value,
                                    QString *errorMessage) {
    QJsonObject object;
    DurationInputPhoneme result;
    if (!readObject(json, object, errorMessage) || !readString(object, "token", result.token, errorMessage)
        || !readBool(object, "onset", result.onset, errorMessage)
        || !readString(object, "language", result.language, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue DurationNote::toJson() const {
    return QJsonObject{{QStringLiteral("position"), position.toJson()},
                       {QStringLiteral("cent"), cent},
                       {QStringLiteral("kind"), kind},
                       {QStringLiteral("pronunciation"), pronunciation},
                       {QStringLiteral("language"), language},
                       {QStringLiteral("phonemes"), dtoListToJson(phonemes)}};
}

bool DurationNote::fromJson(const QJsonValue &json, DurationNote &value, QString *errorMessage) {
    QJsonObject object;
    DurationNote result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "position", result.position, errorMessage)
        || !readInteger(object, "cent", result.cent, errorMessage)
        || !readString(object, "kind", result.kind, errorMessage)
        || !readString(object, "pronunciation", result.pronunciation, errorMessage)
        || !readString(object, "language", result.language, errorMessage)
        || !readDtoList(object, "phonemes", result.phonemes, errorMessage))
        return false;
    if (result.cent < 0 || result.cent > 12800)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field 'cent' must be in [0, 12800]")));
    if (result.kind != QStringLiteral("normal") && result.kind != QStringLiteral("slur"))
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Invalid note kind")));
    if (result.kind == QStringLiteral("slur") && (!result.phonemes.isEmpty() || result.position.gap != 0))
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Slur notes must have no phonemes and no gap")));
    value = std::move(result);
    return true;
}

QJsonValue Mix::toJson() const {
    QJsonArray result;
    for (const auto &row : rows)
        result.append(doubleListToJson(row));
    return result;
}

bool Mix::fromJson(const QJsonValue &json, Mix &value, QString *errorMessage) {
    if (!json.isArray())
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Mix must be an array of arrays")));
    Mix result;
    for (const auto &rowValue : json.toArray()) {
        QList<double> row;
        if (!readDoubleListValue(rowValue, row, errorMessage))
            return false;
        for (double item : row) {
            if (item < 0 || item > 1)
                return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Mix values must be in [0, 1]")));
        }
        result.rows.append(std::move(row));
    }
    value = std::move(result);
    return true;
}

QJsonValue DurationInput::toJson() const {
    return QJsonObject{{QStringLiteral("pieceDuration"), pieceDuration},
                       {QStringLiteral("notes"), dtoListToJson(notes)},
                       {QStringLiteral("mix"), mix.toJson()},
                       {QStringLiteral("mixSampleRate"), mixSampleRate}};
}

bool DurationInput::fromJson(const QJsonValue &json, DurationInput &value, QString *errorMessage) {
    QJsonObject object;
    DurationInput result;
    if (!readObject(json, object, errorMessage)
        || !readNumber(object, "pieceDuration", result.pieceDuration, errorMessage)
        || !readDtoList(object, "notes", result.notes, errorMessage)
        || !readDto(object, "mix", result.mix, errorMessage)
        || !readNumber(object, "mixSampleRate", result.mixSampleRate, errorMessage))
        return false;
    if (result.pieceDuration < 0 || result.mixSampleRate <= 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Duration and sample-rate constraints were violated")));
    value = std::move(result);
    return true;
}

QJsonValue DurationRequest::toJson() const {
    return QJsonObject{{QStringLiteral("context"), context.toJson()},
                       {QStringLiteral("input"), input.toJson()}};
}

bool DurationRequest::fromJson(const QJsonValue &json, DurationRequest &value,
                               QString *errorMessage) {
    QJsonObject object;
    DurationRequest result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "context", result.context, errorMessage)
        || !readDto(object, "input", result.input, errorMessage)
        || !validateMix(result.context, result.input.mix, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ParameterInputPhoneme::toJson() const {
    return QJsonObject{{QStringLiteral("token"), token},
                       {QStringLiteral("onset"), onset},
                       {QStringLiteral("language"), language},
                       {QStringLiteral("start"), start}};
}

bool ParameterInputPhoneme::fromJson(const QJsonValue &json, ParameterInputPhoneme &value,
                                     QString *errorMessage) {
    QJsonObject object;
    ParameterInputPhoneme result;
    if (!readObject(json, object, errorMessage) || !readString(object, "token", result.token, errorMessage)
        || !readBool(object, "onset", result.onset, errorMessage)
        || !readString(object, "language", result.language, errorMessage)
        || !readNumber(object, "start", result.start, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ParameterNote::toJson() const {
    return QJsonObject{{QStringLiteral("position"), position.toJson()},
                       {QStringLiteral("cent"), cent},
                       {QStringLiteral("kind"), kind},
                       {QStringLiteral("pronunciation"), pronunciation},
                       {QStringLiteral("language"), language},
                       {QStringLiteral("phonemes"), dtoListToJson(phonemes)}};
}

bool ParameterNote::fromJson(const QJsonValue &json, ParameterNote &value,
                             QString *errorMessage) {
    QJsonObject object;
    ParameterNote result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "position", result.position, errorMessage)
        || !readInteger(object, "cent", result.cent, errorMessage)
        || !readString(object, "kind", result.kind, errorMessage)
        || !readString(object, "pronunciation", result.pronunciation, errorMessage)
        || !readString(object, "language", result.language, errorMessage)
        || !readDtoList(object, "phonemes", result.phonemes, errorMessage))
        return false;
    if (result.cent < 0 || result.cent > 12800)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field 'cent' must be in [0, 12800]")));
    if (result.kind != QStringLiteral("normal") && result.kind != QStringLiteral("slur"))
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Invalid note kind")));
    if (result.kind == QStringLiteral("slur") && (!result.phonemes.isEmpty() || result.position.gap != 0))
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Slur notes must have no phonemes and no gap")));
    value = std::move(result);
    return true;
}

QJsonValue ParameterRetake::toJson() const {
    return QJsonObject{{QStringLiteral("position"), position}, {QStringLiteral("length"), length}};
}

bool ParameterRetake::fromJson(const QJsonValue &json, ParameterRetake &value,
                               QString *errorMessage) {
    QJsonObject object;
    ParameterRetake result;
    if (!readObject(json, object, errorMessage)
        || !readInteger(object, "position", result.position, errorMessage)
        || !readInteger(object, "length", result.length, errorMessage))
        return false;
    if (result.position < 0 || result.length < 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Retake position and length must be non-negative")));
    value = result;
    return true;
}

QJsonValue Parameter::toJson() const {
    QJsonObject object{{QStringLiteral("values"), doubleListToJson(values)},
                       {QStringLiteral("sampleRate"), sampleRate}};
    if (retake)
        object.insert(QStringLiteral("retake"), retake->toJson());
    return object;
}

bool Parameter::fromJson(const QJsonValue &json, Parameter &value, QString *errorMessage) {
    QJsonObject object;
    Parameter result;
    if (!readObject(json, object, errorMessage) || !readDoubleList(object, "values", result.values, errorMessage)
        || !readNumber(object, "sampleRate", result.sampleRate, errorMessage))
        return false;
    if (result.sampleRate <= 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field 'sampleRate' must be positive")));
    if (const auto it = object.constFind(QStringLiteral("retake")); it != object.constEnd()) {
        ParameterRetake parsed;
        if (!ParameterRetake::fromJson(*it, parsed, errorMessage))
            return false;
        result.retake = parsed;
    }
    value = std::move(result);
    return true;
}

QJsonValue AudioParameter::toJson() const {
    QJsonObject object{{QStringLiteral("sampleRate"), sampleRate}};
    if (values)
        object.insert(QStringLiteral("values"), doubleListToJson(*values));
    return object;
}

bool AudioParameter::fromJson(const QJsonValue &json, AudioParameter &value,
                              QString *errorMessage) {
    QJsonObject object;
    AudioParameter result;
    if (!readObject(json, object, errorMessage)
        || !readNumber(object, "sampleRate", result.sampleRate, errorMessage))
        return false;
    if (result.sampleRate <= 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field 'sampleRate' must be positive")));
    if (const auto it = object.constFind(QStringLiteral("values")); it != object.constEnd()) {
        QList<double> parsed;
        if (!readDoubleListValue(*it, parsed, errorMessage))
            return false;
        result.values = std::move(parsed);
    }
    value = std::move(result);
    return true;
}

QJsonValue ParameterMap::toJson() const { return dtoMapToJson(values); }

bool ParameterMap::fromJson(const QJsonValue &json, ParameterMap &value, QString *errorMessage) {
    ParameterMap result;
    if (!readDtoMapValue(json, result.values, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue AudioParameterMap::toJson() const { return dtoMapToJson(values); }

bool AudioParameterMap::fromJson(const QJsonValue &json, AudioParameterMap &value,
                                 QString *errorMessage) {
    AudioParameterMap result;
    if (!readDtoMapValue(json, result.values, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ParameterInput::toJson() const {
    return QJsonObject{{QStringLiteral("pieceDuration"), pieceDuration},
                       {QStringLiteral("notes"), dtoListToJson(notes)},
                       {QStringLiteral("mix"), mix.toJson()},
                       {QStringLiteral("mixSampleRate"), mixSampleRate},
                       {QStringLiteral("parameters"), parameters.toJson()}};
}

bool ParameterInput::fromJson(const QJsonValue &json, ParameterInput &value,
                              QString *errorMessage) {
    QJsonObject object;
    ParameterInput result;
    if (!readObject(json, object, errorMessage)
        || !readNumber(object, "pieceDuration", result.pieceDuration, errorMessage)
        || !readDtoList(object, "notes", result.notes, errorMessage)
        || !readDto(object, "mix", result.mix, errorMessage)
        || !readNumber(object, "mixSampleRate", result.mixSampleRate, errorMessage)
        || !readDto(object, "parameters", result.parameters, errorMessage))
        return false;
    if (result.pieceDuration < 0 || result.mixSampleRate <= 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Parameter input constraints were violated")));
    value = std::move(result);
    return true;
}

QJsonValue ParameterRequest::toJson() const {
    return QJsonObject{{QStringLiteral("context"), context.toJson()},
                       {QStringLiteral("input"), input.toJson()}};
}

bool ParameterRequest::fromJson(const QJsonValue &json, ParameterRequest &value,
                                QString *errorMessage) {
    QJsonObject object;
    ParameterRequest result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "context", result.context, errorMessage)
        || !readDto(object, "input", result.input, errorMessage)
        || !validateMix(result.context, result.input.mix, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue AudioInput::toJson() const {
    return QJsonObject{{QStringLiteral("pieceDuration"), pieceDuration},
                       {QStringLiteral("notes"), dtoListToJson(notes)},
                       {QStringLiteral("mix"), mix.toJson()},
                       {QStringLiteral("mixSampleRate"), mixSampleRate},
                       {QStringLiteral("parameters"), parameters.toJson()}};
}

bool AudioInput::fromJson(const QJsonValue &json, AudioInput &value, QString *errorMessage) {
    QJsonObject object;
    AudioInput result;
    if (!readObject(json, object, errorMessage)
        || !readNumber(object, "pieceDuration", result.pieceDuration, errorMessage)
        || !readDtoList(object, "notes", result.notes, errorMessage)
        || !readDto(object, "mix", result.mix, errorMessage)
        || !readNumber(object, "mixSampleRate", result.mixSampleRate, errorMessage)
        || !readDto(object, "parameters", result.parameters, errorMessage))
        return false;
    if (result.pieceDuration < 0 || result.mixSampleRate <= 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Audio input constraints were violated")));
    value = std::move(result);
    return true;
}

QJsonValue AudioRequest::toJson() const {
    return QJsonObject{{QStringLiteral("context"), context.toJson()},
                       {QStringLiteral("input"), input.toJson()},
                       {QStringLiteral("acceptableFormats"), stringListToJson(acceptableFormats)},
                       {QStringLiteral("acceptableSchemes"), stringListToJson(acceptableSchemes)}};
}

bool AudioRequest::fromJson(const QJsonValue &json, AudioRequest &value,
                            QString *errorMessage) {
    QJsonObject object;
    AudioRequest result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "context", result.context, errorMessage)
        || !readDto(object, "input", result.input, errorMessage)
        || !validateMix(result.context, result.input.mix, errorMessage))
        return false;
    for (const auto &[key, target] : {std::pair{"acceptableFormats", &result.acceptableFormats}, std::pair{"acceptableSchemes", &result.acceptableSchemes}}) {
        if (object.contains(QLatin1StringView(key)) && !readStringList(object, key, *target, errorMessage))
            return false;
    }
    value = std::move(result);
    return true;
}

QJsonValue Pronunciation::toJson() const {
    return QJsonObject{{QStringLiteral("pronunciation"), pronunciation},
                       {QStringLiteral("candidates"), stringListToJson(candidates)}};
}

bool Pronunciation::fromJson(const QJsonValue &json, Pronunciation &value,
                             QString *errorMessage) {
    QJsonObject object;
    Pronunciation result;
    if (!readObject(json, object, errorMessage)
        || !readString(object, "pronunciation", result.pronunciation, errorMessage)
        || !readStringList(object, "candidates", result.candidates, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue PronunciationOutput::toJson() const {
    return QJsonObject{{QStringLiteral("notes"), dtoListToJson(notes)}};
}

bool PronunciationOutput::fromJson(const QJsonValue &json, PronunciationOutput &value,
                                   QString *errorMessage) {
    QJsonObject object;
    PronunciationOutput result;
    if (!readObject(json, object, errorMessage) || !readDtoList(object, "notes", result.notes, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue PronunciationResponse::toJson() const {
    return QJsonObject{{QStringLiteral("output"), output.toJson()}};
}

bool PronunciationResponse::fromJson(const QJsonValue &json, PronunciationResponse &value,
                                     QString *errorMessage) {
    QJsonObject object;
    PronunciationResponse result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue Phoneme::toJson() const {
    return QJsonObject{{QStringLiteral("token"), token}, {QStringLiteral("onset"), onset}};
}

bool Phoneme::fromJson(const QJsonValue &json, Phoneme &value, QString *errorMessage) {
    QJsonObject object;
    Phoneme result;
    if (!readObject(json, object, errorMessage) || !readString(object, "token", result.token, errorMessage)
        || !readBool(object, "onset", result.onset, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue PhonemeNote::toJson() const {
    return QJsonObject{{QStringLiteral("phonemes"), dtoListToJson(phonemes)}};
}

bool PhonemeNote::fromJson(const QJsonValue &json, PhonemeNote &value,
                           QString *errorMessage) {
    QJsonObject object;
    PhonemeNote result;
    if (!readObject(json, object, errorMessage)
        || !readDtoList(object, "phonemes", result.phonemes, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue PhonemeOutput::toJson() const {
    return QJsonObject{{QStringLiteral("notes"), dtoListToJson(notes)}};
}

bool PhonemeOutput::fromJson(const QJsonValue &json, PhonemeOutput &value,
                             QString *errorMessage) {
    QJsonObject object;
    PhonemeOutput result;
    if (!readObject(json, object, errorMessage) || !readDtoList(object, "notes", result.notes, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue PhonemeResponse::toJson() const {
    return QJsonObject{{QStringLiteral("output"), output.toJson()}};
}

bool PhonemeResponse::fromJson(const QJsonValue &json, PhonemeResponse &value,
                               QString *errorMessage) {
    QJsonObject object;
    PhonemeResponse result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue DurationOutputPhoneme::toJson() const {
    return QJsonObject{{QStringLiteral("start"), start}};
}

bool DurationOutputPhoneme::fromJson(const QJsonValue &json, DurationOutputPhoneme &value,
                                     QString *errorMessage) {
    QJsonObject object;
    DurationOutputPhoneme result;
    if (!readObject(json, object, errorMessage) || !readNumber(object, "start", result.start, errorMessage))
        return false;
    value = result;
    return true;
}

QJsonValue DurationOutputNote::toJson() const {
    return QJsonObject{{QStringLiteral("phonemes"), dtoListToJson(phonemes)}};
}

bool DurationOutputNote::fromJson(const QJsonValue &json, DurationOutputNote &value,
                                  QString *errorMessage) {
    QJsonObject object;
    DurationOutputNote result;
    if (!readObject(json, object, errorMessage)
        || !readDtoList(object, "phonemes", result.phonemes, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue DurationOutput::toJson() const {
    return QJsonObject{{QStringLiteral("notes"), dtoListToJson(notes)}};
}

bool DurationOutput::fromJson(const QJsonValue &json, DurationOutput &value,
                              QString *errorMessage) {
    QJsonObject object;
    DurationOutput result;
    if (!readObject(json, object, errorMessage) || !readDtoList(object, "notes", result.notes, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue DurationResponse::toJson() const {
    return QJsonObject{{QStringLiteral("output"), output.toJson()}};
}

bool DurationResponse::fromJson(const QJsonValue &json, DurationResponse &value,
                                QString *errorMessage) {
    QJsonObject object;
    DurationResponse result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ParameterOutputParameter::toJson() const {
    return QJsonObject{{QStringLiteral("values"), doubleListToJson(values)},
                       {QStringLiteral("sampleRate"), sampleRate}};
}

bool ParameterOutputParameter::fromJson(const QJsonValue &json, ParameterOutputParameter &value,
                                        QString *errorMessage) {
    QJsonObject object;
    ParameterOutputParameter result;
    if (!readObject(json, object, errorMessage) || !readDoubleList(object, "values", result.values, errorMessage)
        || !readNumber(object, "sampleRate", result.sampleRate, errorMessage))
        return false;
    if (result.sampleRate <= 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field 'sampleRate' must be positive")));
    value = std::move(result);
    return true;
}

QJsonValue ParameterOutput::toJson() const {
    return QJsonObject{{QStringLiteral("parameters"), dtoMapToJson(parameters)}};
}

bool ParameterOutput::fromJson(const QJsonValue &json, ParameterOutput &value,
                               QString *errorMessage) {
    QJsonObject object;
    ParameterOutput result;
    if (!readObject(json, object, errorMessage)
        || !readDtoMap(object, "parameters", result.parameters, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ParameterResponse::toJson() const {
    return QJsonObject{{QStringLiteral("output"), output.toJson()}};
}

bool ParameterResponse::fromJson(const QJsonValue &json, ParameterResponse &value,
                                 QString *errorMessage) {
    QJsonObject object;
    ParameterResponse result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue AudioResponse::toJson() const {
    return QJsonObject{{QStringLiteral("output"), output.toJson()}};
}

bool AudioResponse::fromJson(const QJsonValue &json, AudioResponse &value,
                             QString *errorMessage) {
    QJsonObject object;
    AudioResponse result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ExtractorInfo::toJson() const {
    QJsonArray sampleRates;
    for (int sampleRate : acceptableAudioSampleRates)
        sampleRates.append(sampleRate);
    return QJsonObject{{QStringLiteral("id"), id},
                       {QStringLiteral("name"), name},
                       {QStringLiteral("preferredAudioSampleRate"), preferredAudioSampleRate},
                       {QStringLiteral("acceptableFormats"), stringListToJson(acceptableFormats)},
                       {QStringLiteral("acceptableSchemes"), stringListToJson(acceptableSchemes)},
                       {QStringLiteral("acceptableAudioSampleRates"), sampleRates}};
}

bool ExtractorInfo::fromJson(const QJsonValue &json, ExtractorInfo &value, QString *errorMessage) {
    QJsonObject object;
    ExtractorInfo result;
    if (!readObject(json, object, errorMessage)
        || !readString(object, "id", result.id, errorMessage)
        || !readString(object, "name", result.name, errorMessage)
        || !readInteger(object, "preferredAudioSampleRate", result.preferredAudioSampleRate, errorMessage))
        return false;
    if (result.preferredAudioSampleRate <= 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field '%1' must be positive")).arg(QStringLiteral("preferredAudioSampleRate")));
    for (const auto &[key, target] : {std::pair{"acceptableFormats", &result.acceptableFormats}, std::pair{"acceptableSchemes", &result.acceptableSchemes}}) {
        if (object.contains(QLatin1StringView(key)) && !readStringList(object, key, *target, errorMessage))
            return false;
    }
    if (const auto it = object.constFind(QStringLiteral("acceptableAudioSampleRates")); it != object.constEnd()) {
        if (!it->isArray())
            return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Expected a JSON array")));
        for (const auto &item : it->toArray()) {
            const double sampleRate = item.toDouble();
            if (!item.isDouble() || !std::isfinite(sampleRate) || sampleRate <= 0
                || std::trunc(sampleRate) != sampleRate || sampleRate > std::numeric_limits<int>::max())
                return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Array item must be a positive integer")));
            result.acceptableAudioSampleRates.append(static_cast<int>(sampleRate));
        }
    }
    value = std::move(result);
    return true;
}

QJsonValue SeparationTrackInfo::toJson() const {
    return QJsonObject{{QStringLiteral("name"), name}};
}

bool SeparationTrackInfo::fromJson(const QJsonValue &json, SeparationTrackInfo &value, QString *errorMessage) {
    QJsonObject object;
    SeparationTrackInfo result;
    if (!readObject(json, object, errorMessage)
        || !readString(object, "name", result.name, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue SeparationExtractorInfo::toJson() const {
    auto object = ExtractorInfo::toJson().toObject();
    object.insert(QStringLiteral("tracks"), dtoListToJson(tracks));
    return object;
}

bool SeparationExtractorInfo::fromJson(const QJsonValue &json, SeparationExtractorInfo &value, QString *errorMessage) {
    QJsonObject object;
    SeparationExtractorInfo result;
    if (!readObject(json, object, errorMessage)
        || !ExtractorInfo::fromJson(json, result, errorMessage)
        || !readDtoList(object, "tracks", result.tracks, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ExtractorList::toJson() const {
    return QJsonObject{{QStringLiteral("note"), dtoListToJson(note)},
                       {QStringLiteral("tempo"), dtoListToJson(tempo)},
                       {QStringLiteral("pitch"), dtoListToJson(pitch)},
                       {QStringLiteral("separation"), dtoListToJson(separation)}};
}

bool ExtractorList::fromJson(const QJsonValue &json, ExtractorList &value, QString *errorMessage) {
    QJsonObject object;
    ExtractorList result;
    if (!readObject(json, object, errorMessage)
        || !readDtoList(object, "note", result.note, errorMessage)
        || !readDtoList(object, "tempo", result.tempo, errorMessage)
        || !readDtoList(object, "pitch", result.pitch, errorMessage)
        || !readDtoList(object, "separation", result.separation, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ExtractionInput::toJson() const {
    return QJsonObject{{QStringLiteral("audioUrl"), audioUrl}};
}

bool ExtractionInput::fromJson(const QJsonValue &json, ExtractionInput &value, QString *errorMessage) {
    QJsonObject object;
    ExtractionInput result;
    if (!readObject(json, object, errorMessage) || !readString(object, "audioUrl", result.audioUrl, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ExtractionRequest::toJson() const {
    return QJsonObject{{QStringLiteral("extractor"), extractor}, {QStringLiteral("input"), input.toJson()}};
}

bool ExtractionRequest::fromJson(const QJsonValue &json, ExtractionRequest &value, QString *errorMessage) {
    QJsonObject object;
    ExtractionRequest result;
    if (!readObject(json, object, errorMessage)
        || !readString(object, "extractor", result.extractor, errorMessage)
        || !readDto(object, "input", result.input, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue SeparationExtractionRequest::toJson() const {
    return QJsonObject{{QStringLiteral("extractor"), extractor},
                       {QStringLiteral("input"), input.toJson()},
                       {QStringLiteral("acceptableFormats"), stringListToJson(acceptableFormats)},
                       {QStringLiteral("acceptableSchemes"), stringListToJson(acceptableSchemes)}};
}

bool SeparationExtractionRequest::fromJson(const QJsonValue &json, SeparationExtractionRequest &value, QString *errorMessage) {
    QJsonObject object;
    SeparationExtractionRequest result;
    if (!readObject(json, object, errorMessage)
        || !readString(object, "extractor", result.extractor, errorMessage)
        || !readDto(object, "input", result.input, errorMessage))
        return false;
    for (const auto &[key, target] : {std::pair{"acceptableFormats", &result.acceptableFormats}, std::pair{"acceptableSchemes", &result.acceptableSchemes}}) {
        if (object.contains(QLatin1StringView(key)) && !readStringList(object, key, *target, errorMessage))
            return false;
    }
    value = std::move(result);
    return true;
}

QJsonValue ExtractedNote::toJson() const {
    return QJsonObject{{QStringLiteral("position"), position.toJson()}, {QStringLiteral("cent"), cent}};
}

bool ExtractedNote::fromJson(const QJsonValue &json, ExtractedNote &value, QString *errorMessage) {
    QJsonObject object;
    ExtractedNote result;
    if (!readObject(json, object, errorMessage)
        || !readDto(object, "position", result.position, errorMessage)
        || !readInteger(object, "cent", result.cent, errorMessage))
        return false;
    if (result.cent < 0 || result.cent > 12800)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field 'cent' must be in [0, 12800]")));
    value = result;
    return true;
}

QJsonValue NoteExtractionOutput::toJson() const {
    return QJsonObject{{QStringLiteral("notes"), dtoListToJson(notes)}};
}

bool NoteExtractionOutput::fromJson(const QJsonValue &json, NoteExtractionOutput &value, QString *errorMessage) {
    QJsonObject object;
    NoteExtractionOutput result;
    if (!readObject(json, object, errorMessage) || !readDtoList(object, "notes", result.notes, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue NoteExtractionResponse::toJson() const {
    return QJsonObject{{QStringLiteral("output"), output.toJson()}};
}

bool NoteExtractionResponse::fromJson(const QJsonValue &json, NoteExtractionResponse &value, QString *errorMessage) {
    QJsonObject object;
    NoteExtractionResponse result;
    if (!readObject(json, object, errorMessage) || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ExtractedBeat::toJson() const {
    return QJsonObject{{QStringLiteral("position"), position}, {QStringLiteral("downbeat"), downbeat}};
}

bool ExtractedBeat::fromJson(const QJsonValue &json, ExtractedBeat &value, QString *errorMessage) {
    QJsonObject object;
    ExtractedBeat result;
    if (!readObject(json, object, errorMessage)
        || !readNumber(object, "position", result.position, errorMessage)
        || !readBool(object, "downbeat", result.downbeat, errorMessage))
        return false;
    if (result.position < 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field '%1' must be non-negative")).arg(QStringLiteral("position")));
    value = result;
    return true;
}

QJsonValue TempoExtractionOutput::toJson() const {
    return QJsonObject{{QStringLiteral("beats"), dtoListToJson(beats)}};
}

bool TempoExtractionOutput::fromJson(const QJsonValue &json, TempoExtractionOutput &value, QString *errorMessage) {
    QJsonObject object;
    TempoExtractionOutput result;
    if (!readObject(json, object, errorMessage) || !readDtoList(object, "beats", result.beats, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue TempoExtractionResponse::toJson() const {
    return QJsonObject{{QStringLiteral("output"), output.toJson()}};
}

bool TempoExtractionResponse::fromJson(const QJsonValue &json, TempoExtractionResponse &value, QString *errorMessage) {
    QJsonObject object;
    TempoExtractionResponse result;
    if (!readObject(json, object, errorMessage) || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue PitchExtractionSegment::toJson() const {
    return QJsonObject{{QStringLiteral("gap"), gap}, {QStringLiteral("pitch"), doubleListToJson(pitch)}};
}

bool PitchExtractionSegment::fromJson(const QJsonValue &json, PitchExtractionSegment &value, QString *errorMessage) {
    QJsonObject object;
    PitchExtractionSegment result;
    if (!readObject(json, object, errorMessage)
        || !readInteger(object, "gap", result.gap, errorMessage)
        || !readDoubleList(object, "pitch", result.pitch, errorMessage))
        return false;
    if (result.gap < 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field '%1' must be non-negative")).arg(QStringLiteral("gap")));
    if (result.pitch.isEmpty())
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field '%1' must contain at least one item")).arg(QStringLiteral("pitch")));
    for (double pitch : result.pitch) {
        if (pitch < 0 || pitch > 12800)
            return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Pitch values must be in [0, 12800]")));
    }
    value = std::move(result);
    return true;
}

QJsonValue PitchExtractionOutput::toJson() const {
    return QJsonObject{{QStringLiteral("segments"), dtoListToJson(segments)}, {QStringLiteral("sampleRate"), sampleRate}};
}

bool PitchExtractionOutput::fromJson(const QJsonValue &json, PitchExtractionOutput &value, QString *errorMessage) {
    QJsonObject object;
    PitchExtractionOutput result;
    if (!readObject(json, object, errorMessage)
        || !readDtoList(object, "segments", result.segments, errorMessage)
        || !readNumber(object, "sampleRate", result.sampleRate, errorMessage))
        return false;
    if (result.sampleRate <= 0)
        return fail(errorMessage, translateError(QT_TRANSLATE_NOOP("Synth::Internal::Api::Dtos", "Field 'sampleRate' must be positive")));
    value = std::move(result);
    return true;
}

QJsonValue PitchExtractionResponse::toJson() const {
    return QJsonObject{{QStringLiteral("output"), output.toJson()}};
}

bool PitchExtractionResponse::fromJson(const QJsonValue &json, PitchExtractionResponse &value, QString *errorMessage) {
    QJsonObject object;
    PitchExtractionResponse result;
    if (!readObject(json, object, errorMessage) || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue SeparationTrack::toJson() const {
    return QJsonObject{{QStringLiteral("name"), name}, {QStringLiteral("audioUrl"), audioUrl}};
}

bool SeparationTrack::fromJson(const QJsonValue &json, SeparationTrack &value, QString *errorMessage) {
    QJsonObject object;
    SeparationTrack result;
    if (!readObject(json, object, errorMessage)
        || !readString(object, "name", result.name, errorMessage)
        || !readString(object, "audioUrl", result.audioUrl, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue SeparationExtractionOutput::toJson() const {
    return QJsonObject{{QStringLiteral("tracks"), dtoListToJson(tracks)}};
}

bool SeparationExtractionOutput::fromJson(const QJsonValue &json, SeparationExtractionOutput &value, QString *errorMessage) {
    QJsonObject object;
    SeparationExtractionOutput result;
    if (!readObject(json, object, errorMessage) || !readDtoList(object, "tracks", result.tracks, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue SeparationExtractionResponse::toJson() const {
    return QJsonObject{{QStringLiteral("output"), output.toJson()}};
}

bool SeparationExtractionResponse::fromJson(const QJsonValue &json, SeparationExtractionResponse &value, QString *errorMessage) {
    QJsonObject object;
    SeparationExtractionResponse result;
    if (!readObject(json, object, errorMessage) || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

} // namespace Synth::Internal::Api::V1
