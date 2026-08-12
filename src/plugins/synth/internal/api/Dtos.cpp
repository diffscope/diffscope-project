#include "Dtos.h"

#include <cmath>
#include <limits>

#include <QJsonDocument>

namespace Synth::Internal::Api::V1 {
namespace {

    bool fail(QString *errorMessage, const QString &message) {
        if (errorMessage)
            *errorMessage = message;
        return false;
    }

    bool readObject(const QJsonValue &json, QJsonObject &object, QString *errorMessage) {
        if (!json.isObject())
            return fail(errorMessage, QStringLiteral("Expected a JSON object."));
        object = json.toObject();
        return true;
    }

    bool requiredValue(const QJsonObject &object, const char *key, QJsonValue &value,
                       QString *errorMessage) {
        const QLatin1StringView keyView(key);
        const auto it = object.constFind(keyView);
        if (it == object.constEnd())
            return fail(errorMessage, QStringLiteral("Missing required field '%1'.").arg(keyView));
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
                        QStringLiteral("Field '%1' must be a string.").arg(QLatin1StringView(key)));
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
                        QStringLiteral("Field '%1' must be a boolean.").arg(QLatin1StringView(key)));
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
                        QStringLiteral("Field '%1' must be a finite number.")
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
                        QStringLiteral("Field '%1' must be an integer.").arg(QLatin1StringView(key)));
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
            return fail(errorMessage, QStringLiteral("Expected an array of strings."));
        QStringList result;
        const auto array = json.toArray();
        result.reserve(array.size());
        for (const auto &item : array) {
            if (!item.isString())
                return fail(errorMessage, QStringLiteral("Array item must be a string."));
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
            return fail(errorMessage, QStringLiteral("Expected an array of numbers."));
        QList<double> result;
        const auto array = json.toArray();
        result.reserve(array.size());
        for (const auto &item : array) {
            if (!item.isDouble() || !std::isfinite(item.toDouble()))
                return fail(errorMessage, QStringLiteral("Array item must be a finite number."));
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
            return fail(errorMessage, QStringLiteral("Expected a JSON array."));
        QList<T> result;
        const auto array = json.toArray();
        result.reserve(array.size());
        for (qsizetype i = 0; i < array.size(); ++i) {
            T item;
            QString nestedError;
            if (!T::fromJson(array.at(i), item, &nestedError))
                return fail(errorMessage, QStringLiteral("Invalid array item %1: %2").arg(i).arg(nestedError));
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
            return fail(errorMessage, QStringLiteral("Expected an open JSON object map."));
        QMap<QString, T> result;
        const auto object = json.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            T item;
            QString nestedError;
            if (!T::fromJson(*it, item, &nestedError)) {
                return fail(errorMessage,
                            QStringLiteral("Invalid map value '%1': %2").arg(it.key(), nestedError));
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

    bool readCompleteState(const QJsonObject &object, QString *errorMessage) {
        QString state;
        if (!readString(object, "state", state, errorMessage))
            return false;
        if (state != QStringLiteral("COMPLETE"))
            return fail(errorMessage, QStringLiteral("Field 'state' must be 'COMPLETE'."));
        return true;
    }

    bool readNonStreamingFlag(const QJsonObject &object, QString *errorMessage) {
        const auto it = object.constFind(QStringLiteral("stream"));
        if (it == object.constEnd())
            return true;
        if (!it->isBool() || it->toBool())
            return fail(errorMessage, QStringLiteral("Only non-streaming requests are supported."));
        return true;
    }

    bool validateMix(const MultiSingerContext &context, const Mix &mix,
                     QString *errorMessage) {
        const qsizetype expectedColumns = context.singers.size() - 1;
        for (qsizetype rowIndex = 0; rowIndex < mix.rows.size(); ++rowIndex) {
            const auto &row = mix.rows.at(rowIndex);
            if (row.size() != expectedColumns) {
                return fail(errorMessage,
                            QStringLiteral("Mix row %1 must contain exactly %2 value(s).")
                                .arg(rowIndex)
                                .arg(expectedColumns));
            }
            double sum{};
            for (double value : row)
                sum += value;
            if (sum > 1.0 + 1e-12) {
                return fail(errorMessage,
                            QStringLiteral("Mix row %1 must have a sum no greater than 1.")
                                .arg(rowIndex));
            }
        }
        return true;
    }

} // namespace

QJsonValue ApplicationInfo::toJson() const {
    return QJsonObject{{QStringLiteral("api_version"), apiVersion}};
}

bool ApplicationInfo::fromJson(const QJsonValue &json, ApplicationInfo &value, QString *errorMessage) {
    QJsonObject object;
    ApplicationInfo result;
    if (!readObject(json, object, errorMessage)
        || !readInteger(object, "api_version", result.apiVersion, errorMessage))
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

QJsonValue ArchitectureParameterMetadata::toJson() const {
    QJsonObject object{{QStringLiteral("type"), type == Direct ? QStringLiteral("DIRECT")
                                                               : QStringLiteral("INDIRECT")}};
    if (type == Indirect)
        object.insert(QStringLiteral("depends_on"), stringListToJson(dependsOn));
    return object;
}

bool ArchitectureParameterMetadata::fromJson(const QJsonValue &json,
                                             ArchitectureParameterMetadata &value,
                                             QString *errorMessage) {
    QJsonObject object;
    QString typeValue;
    ArchitectureParameterMetadata result;
    if (!readObject(json, object, errorMessage) || !readString(object, "type", typeValue, errorMessage))
        return false;
    if (typeValue == QStringLiteral("DIRECT")) {
        result.type = Direct;
    } else if (typeValue == QStringLiteral("INDIRECT")) {
        result.type = Indirect;
        if (!readStringList(object, "depends_on", result.dependsOn, errorMessage))
            return false;
    } else {
        return fail(errorMessage, QStringLiteral("Unknown architecture parameter type '%1'.").arg(typeValue));
    }
    value = std::move(result);
    return true;
}

QJsonValue ArchitectureMetadata::toJson() const {
    const auto pronunciation = pronunciationMode == FullPronunciation ? QStringLiteral("FULL")
                                                                      : QStringLiteral("SKIP");
    QString phoneme;
    switch (phonemeMode) {
    case FullPhoneme:
        phoneme = QStringLiteral("FULL");
        break;
    case TokenOnlyPhoneme:
        phoneme = QStringLiteral("TOKEN_ONLY");
        break;
    case SkipPhoneme:
        phoneme = QStringLiteral("SKIP");
        break;
    }
    return QJsonObject{{QStringLiteral("id"), id},
                       {QStringLiteral("name"), name},
                       {QStringLiteral("pronunciation_mode"), pronunciation},
                       {QStringLiteral("phoneme_mode"), phoneme},
                       {QStringLiteral("parameters"), dtoMapToJson(parameters)},
                       {QStringLiteral("audio_dependencies"), stringListToJson(audioDependencies)}};
}

bool ArchitectureMetadata::fromJson(const QJsonValue &json, ArchitectureMetadata &value,
                                    QString *errorMessage) {
    QJsonObject object;
    QString pronunciation;
    QString phoneme;
    ArchitectureMetadata result;
    if (!readObject(json, object, errorMessage) || !readString(object, "id", result.id, errorMessage)
        || !readString(object, "name", result.name, errorMessage)
        || !readString(object, "pronunciation_mode", pronunciation, errorMessage)
        || !readString(object, "phoneme_mode", phoneme, errorMessage)
        || !readDtoMap(object, "parameters", result.parameters, errorMessage)
        || !readStringList(object, "audio_dependencies", result.audioDependencies, errorMessage))
        return false;

    if (pronunciation == QStringLiteral("FULL"))
        result.pronunciationMode = FullPronunciation;
    else if (pronunciation == QStringLiteral("SKIP"))
        result.pronunciationMode = SkipPronunciation;
    else
        return fail(errorMessage, QStringLiteral("Unknown pronunciation_mode '%1'.").arg(pronunciation));

    if (phoneme == QStringLiteral("FULL"))
        result.phonemeMode = FullPhoneme;
    else if (phoneme == QStringLiteral("TOKEN_ONLY"))
        result.phonemeMode = TokenOnlyPhoneme;
    else if (phoneme == QStringLiteral("SKIP"))
        result.phonemeMode = SkipPhoneme;
    else
        return fail(errorMessage, QStringLiteral("Unknown phoneme_mode '%1'.").arg(phoneme));

    value = std::move(result);
    return true;
}

QJsonValue ArchitectureMetadataList::toJson() const { return dtoListToJson(items); }

bool ArchitectureMetadataList::fromJson(const QJsonValue &json, ArchitectureMetadataList &value,
                                         QString *errorMessage) {
    ArchitectureMetadataList result;
    if (!readDtoListValue(json, result.items, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue SingerLanguageInfo::toJson() const {
    return QJsonObject{{QStringLiteral("name"), name},
                       {QStringLiteral("default_lyric"), defaultLyric}};
}

bool SingerLanguageInfo::fromJson(const QJsonValue &json, SingerLanguageInfo &value,
                                  QString *errorMessage) {
    QJsonObject object;
    SingerLanguageInfo result;
    if (!readObject(json, object, errorMessage)
        || !readString(object, "name", result.name, errorMessage)
        || !readString(object, "default_lyric", result.defaultLyric, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue SingerInfo::toJson() const {
    return QJsonObject{{QStringLiteral("id"), id},
                       {QStringLiteral("name"), name},
                       {QStringLiteral("arch"), arch},
                       {QStringLiteral("mix_group"), mixGroup},
                       {QStringLiteral("languages"), dtoMapToJson(languages)},
                       {QStringLiteral("default_language"), defaultLanguage},
                       {QStringLiteral("arch_specific_info"), archSpecificInfo},
                       {QStringLiteral("default_extra"), defaultExtra}};
}

bool SingerInfo::fromJson(const QJsonValue &json, SingerInfo &value, QString *errorMessage) {
    QJsonObject object;
    SingerInfo result;
    if (!readObject(json, object, errorMessage) || !readString(object, "id", result.id, errorMessage)
        || !readString(object, "name", result.name, errorMessage)
        || !readString(object, "arch", result.arch, errorMessage)
        || !readString(object, "mix_group", result.mixGroup, errorMessage)
        || !readDtoMap(object, "languages", result.languages, errorMessage)
        || !readString(object, "default_language", result.defaultLanguage, errorMessage)
        || !readAny(object, "arch_specific_info", result.archSpecificInfo, errorMessage)
        || !readAny(object, "default_extra", result.defaultExtra, errorMessage))
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

SYNTH_SIMPLE_STRING_DTO(SingerAvatarResponse, avatarUrl, "avatar_url")
SYNTH_SIMPLE_STRING_DTO(SingerBackgroundResponse, backgroundUrl, "background_url")
SYNTH_SIMPLE_STRING_DTO(EnvTagResponse, envTag, "env_tag")
SYNTH_SIMPLE_STRING_DTO(AudioOutput, audioUrl, "audio_url")

#undef SYNTH_SIMPLE_STRING_DTO

QJsonValue SingerDemoAudio::toJson() const {
    return QJsonObject{{QStringLiteral("name"), name}, {QStringLiteral("audio_url"), audioUrl}};
}

bool SingerDemoAudio::fromJson(const QJsonValue &json, SingerDemoAudio &value,
                               QString *errorMessage) {
    QJsonObject object;
    SingerDemoAudio result;
    if (!readObject(json, object, errorMessage) || !readString(object, "name", result.name, errorMessage)
        || !readString(object, "audio_url", result.audioUrl, errorMessage))
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
                       {QStringLiteral("arch_extra"), archExtra},
                       {QStringLiteral("singer"), singer.toJson()}};
}

bool SingleSingerContext::fromJson(const QJsonValue &json, SingleSingerContext &value,
                                   QString *errorMessage) {
    QJsonObject object;
    SingleSingerContext result;
    if (!readObject(json, object, errorMessage) || !readString(object, "arch", result.arch, errorMessage)
        || !readAny(object, "arch_extra", result.archExtra, errorMessage)
        || !readDto(object, "singer", result.singer, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue MultiSingerContext::toJson() const {
    return QJsonObject{{QStringLiteral("arch"), arch},
                       {QStringLiteral("arch_extra"), archExtra},
                       {QStringLiteral("singers"), dtoListToJson(singers)}};
}

bool MultiSingerContext::fromJson(const QJsonValue &json, MultiSingerContext &value,
                                  QString *errorMessage) {
    QJsonObject object;
    MultiSingerContext result;
    if (!readObject(json, object, errorMessage) || !readString(object, "arch", result.arch, errorMessage)
        || !readAny(object, "arch_extra", result.archExtra, errorMessage)
        || !readDtoList(object, "singers", result.singers, errorMessage))
        return false;
    if (result.singers.isEmpty())
        return fail(errorMessage, QStringLiteral("Field 'singers' must contain at least one item."));
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
    if (!readObject(json, object, errorMessage) || !readNonStreamingFlag(object, errorMessage)
        || !readDto(object, "context", result.context, errorMessage)
        || !readDto(object, "input", result.input, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue PronunciationNote::toJson() const {
    return QJsonObject{{QStringLiteral("pronunciation"), pronunciation},
                       {QStringLiteral("language"), language}};
}

bool PronunciationNote::fromJson(const QJsonValue &json, PronunciationNote &value,
                                 QString *errorMessage) {
    QJsonObject object;
    PronunciationNote result;
    if (!readObject(json, object, errorMessage)
        || !readString(object, "pronunciation", result.pronunciation, errorMessage)
        || !readString(object, "language", result.language, errorMessage))
        return false;
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
    if (!readObject(json, object, errorMessage) || !readNonStreamingFlag(object, errorMessage)
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
        return fail(errorMessage, QStringLiteral("Note position values must be non-negative."));
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
        || !readString(object, "pronunciation", result.pronunciation, errorMessage)
        || !readString(object, "language", result.language, errorMessage)
        || !readDtoList(object, "phonemes", result.phonemes, errorMessage))
        return false;
    if (result.cent < 0 || result.cent > 12800)
        return fail(errorMessage, QStringLiteral("Field 'cent' must be in [0, 12800]."));
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
        return fail(errorMessage, QStringLiteral("Mix must be an array of arrays."));
    Mix result;
    for (const auto &rowValue : json.toArray()) {
        QList<double> row;
        if (!readDoubleListValue(rowValue, row, errorMessage))
            return false;
        for (double item : row) {
            if (item < 0 || item > 1)
                return fail(errorMessage, QStringLiteral("Mix values must be in [0, 1]."));
        }
        result.rows.append(std::move(row));
    }
    value = std::move(result);
    return true;
}

QJsonValue DurationInput::toJson() const {
    return QJsonObject{{QStringLiteral("piece_duration"), pieceDuration},
                       {QStringLiteral("notes"), dtoListToJson(notes)},
                       {QStringLiteral("mix"), mix.toJson()},
                       {QStringLiteral("mix_sample_rate"), mixSampleRate}};
}

bool DurationInput::fromJson(const QJsonValue &json, DurationInput &value, QString *errorMessage) {
    QJsonObject object;
    DurationInput result;
    if (!readObject(json, object, errorMessage)
        || !readNumber(object, "piece_duration", result.pieceDuration, errorMessage)
        || !readDtoList(object, "notes", result.notes, errorMessage)
        || !readDto(object, "mix", result.mix, errorMessage)
        || !readNumber(object, "mix_sample_rate", result.mixSampleRate, errorMessage))
        return false;
    if (result.pieceDuration < 0 || result.mixSampleRate <= 0)
        return fail(errorMessage, QStringLiteral("Duration and sample-rate constraints were violated."));
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
    if (!readObject(json, object, errorMessage) || !readNonStreamingFlag(object, errorMessage)
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
        || !readString(object, "pronunciation", result.pronunciation, errorMessage)
        || !readString(object, "language", result.language, errorMessage)
        || !readDtoList(object, "phonemes", result.phonemes, errorMessage))
        return false;
    if (result.cent < 0 || result.cent > 12800)
        return fail(errorMessage, QStringLiteral("Field 'cent' must be in [0, 12800]."));
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
        return fail(errorMessage, QStringLiteral("Retake position and length must be non-negative."));
    value = result;
    return true;
}

QJsonValue Parameter::toJson() const {
    QJsonObject object{{QStringLiteral("values"), doubleListToJson(values)},
                       {QStringLiteral("sample_rate"), sampleRate}};
    if (retake)
        object.insert(QStringLiteral("retake"), retake->toJson());
    return object;
}

bool Parameter::fromJson(const QJsonValue &json, Parameter &value, QString *errorMessage) {
    QJsonObject object;
    Parameter result;
    if (!readObject(json, object, errorMessage) || !readDoubleList(object, "values", result.values, errorMessage)
        || !readNumber(object, "sample_rate", result.sampleRate, errorMessage))
        return false;
    if (result.sampleRate <= 0)
        return fail(errorMessage, QStringLiteral("Field 'sample_rate' must be positive."));
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
    QJsonObject object{{QStringLiteral("sample_rate"), sampleRate}};
    if (values)
        object.insert(QStringLiteral("values"), doubleListToJson(*values));
    return object;
}

bool AudioParameter::fromJson(const QJsonValue &json, AudioParameter &value,
                              QString *errorMessage) {
    QJsonObject object;
    AudioParameter result;
    if (!readObject(json, object, errorMessage)
        || !readNumber(object, "sample_rate", result.sampleRate, errorMessage))
        return false;
    if (result.sampleRate <= 0)
        return fail(errorMessage, QStringLiteral("Field 'sample_rate' must be positive."));
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
    return QJsonObject{{QStringLiteral("piece_duration"), pieceDuration},
                       {QStringLiteral("notes"), dtoListToJson(notes)},
                       {QStringLiteral("mix"), mix.toJson()},
                       {QStringLiteral("mix_sample_rate"), mixSampleRate},
                       {QStringLiteral("parameters"), parameters.toJson()}};
}

bool ParameterInput::fromJson(const QJsonValue &json, ParameterInput &value,
                              QString *errorMessage) {
    QJsonObject object;
    ParameterInput result;
    if (!readObject(json, object, errorMessage)
        || !readNumber(object, "piece_duration", result.pieceDuration, errorMessage)
        || !readDtoList(object, "notes", result.notes, errorMessage)
        || !readDto(object, "mix", result.mix, errorMessage)
        || !readNumber(object, "mix_sample_rate", result.mixSampleRate, errorMessage)
        || !readDto(object, "parameters", result.parameters, errorMessage))
        return false;
    if (result.pieceDuration < 0 || result.mixSampleRate <= 0)
        return fail(errorMessage, QStringLiteral("Parameter input constraints were violated."));
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
    if (!readObject(json, object, errorMessage) || !readNonStreamingFlag(object, errorMessage)
        || !readDto(object, "context", result.context, errorMessage)
        || !readDto(object, "input", result.input, errorMessage)
        || !validateMix(result.context, result.input.mix, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue AudioInput::toJson() const {
    return QJsonObject{{QStringLiteral("piece_duration"), pieceDuration},
                       {QStringLiteral("notes"), dtoListToJson(notes)},
                       {QStringLiteral("mix"), mix.toJson()},
                       {QStringLiteral("mix_sample_rate"), mixSampleRate},
                       {QStringLiteral("parameters"), parameters.toJson()}};
}

bool AudioInput::fromJson(const QJsonValue &json, AudioInput &value, QString *errorMessage) {
    QJsonObject object;
    AudioInput result;
    if (!readObject(json, object, errorMessage)
        || !readNumber(object, "piece_duration", result.pieceDuration, errorMessage)
        || !readDtoList(object, "notes", result.notes, errorMessage)
        || !readDto(object, "mix", result.mix, errorMessage)
        || !readNumber(object, "mix_sample_rate", result.mixSampleRate, errorMessage)
        || !readDto(object, "parameters", result.parameters, errorMessage))
        return false;
    if (result.pieceDuration < 0 || result.mixSampleRate <= 0)
        return fail(errorMessage, QStringLiteral("Audio input constraints were violated."));
    value = std::move(result);
    return true;
}

QJsonValue AudioRequest::toJson() const {
    return QJsonObject{{QStringLiteral("context"), context.toJson()},
                       {QStringLiteral("input"), input.toJson()}};
}

bool AudioRequest::fromJson(const QJsonValue &json, AudioRequest &value,
                            QString *errorMessage) {
    QJsonObject object;
    AudioRequest result;
    if (!readObject(json, object, errorMessage) || !readNonStreamingFlag(object, errorMessage)
        || !readDto(object, "context", result.context, errorMessage)
        || !readDto(object, "input", result.input, errorMessage)
        || !validateMix(result.context, result.input.mix, errorMessage))
        return false;
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
    return QJsonObject{{QStringLiteral("state"), QStringLiteral("COMPLETE")},
                       {QStringLiteral("output"), output.toJson()}};
}

bool PronunciationResponse::fromJson(const QJsonValue &json, PronunciationResponse &value,
                                     QString *errorMessage) {
    QJsonObject object;
    PronunciationResponse result;
    if (!readObject(json, object, errorMessage) || !readCompleteState(object, errorMessage)
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
    return QJsonObject{{QStringLiteral("state"), QStringLiteral("COMPLETE")},
                       {QStringLiteral("output"), output.toJson()}};
}

bool PhonemeResponse::fromJson(const QJsonValue &json, PhonemeResponse &value,
                               QString *errorMessage) {
    QJsonObject object;
    PhonemeResponse result;
    if (!readObject(json, object, errorMessage) || !readCompleteState(object, errorMessage)
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
    return QJsonObject{{QStringLiteral("state"), QStringLiteral("COMPLETE")},
                       {QStringLiteral("output"), output.toJson()}};
}

bool DurationResponse::fromJson(const QJsonValue &json, DurationResponse &value,
                                QString *errorMessage) {
    QJsonObject object;
    DurationResponse result;
    if (!readObject(json, object, errorMessage) || !readCompleteState(object, errorMessage)
        || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue ParameterOutputParameter::toJson() const {
    return QJsonObject{{QStringLiteral("values"), doubleListToJson(values)},
                       {QStringLiteral("sample_rate"), sampleRate}};
}

bool ParameterOutputParameter::fromJson(const QJsonValue &json, ParameterOutputParameter &value,
                                        QString *errorMessage) {
    QJsonObject object;
    ParameterOutputParameter result;
    if (!readObject(json, object, errorMessage) || !readDoubleList(object, "values", result.values, errorMessage)
        || !readNumber(object, "sample_rate", result.sampleRate, errorMessage))
        return false;
    if (result.sampleRate <= 0)
        return fail(errorMessage, QStringLiteral("Field 'sample_rate' must be positive."));
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
    return QJsonObject{{QStringLiteral("state"), QStringLiteral("COMPLETE")},
                       {QStringLiteral("output"), output.toJson()}};
}

bool ParameterResponse::fromJson(const QJsonValue &json, ParameterResponse &value,
                                 QString *errorMessage) {
    QJsonObject object;
    ParameterResponse result;
    if (!readObject(json, object, errorMessage) || !readCompleteState(object, errorMessage)
        || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

QJsonValue AudioResponse::toJson() const {
    return QJsonObject{{QStringLiteral("state"), QStringLiteral("COMPLETE")},
                       {QStringLiteral("output"), output.toJson()}};
}

bool AudioResponse::fromJson(const QJsonValue &json, AudioResponse &value,
                             QString *errorMessage) {
    QJsonObject object;
    AudioResponse result;
    if (!readObject(json, object, errorMessage) || !readCompleteState(object, errorMessage)
        || !readDto(object, "output", result.output, errorMessage))
        return false;
    value = std::move(result);
    return true;
}

} // namespace Synth::Internal::Api::V1
