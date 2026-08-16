// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisTaskCodec.h"

#include <xxhash.h>

#include <utility>

#include <QCborValue>
#include <QJsonArray>
#include <QJsonDocument>

namespace Synth::Internal::TaskCodec {

    namespace {

        QJsonArray doublesToJson(const QList<double> &values) {
            QJsonArray result;
            for (double value : values) {
                result.append(value);
            }
            return result;
        }

        QList<double> doublesFromJson(const QJsonValue &value) {
            QList<double> result;
            for (const auto &item : value.toArray()) {
                result.append(item.toDouble());
            }
            return result;
        }

        QJsonObject singerToJson(const SynthesisSinger &singer) {
            return {
                {QStringLiteral("id"), singer.id},
                {QStringLiteral("extra"), singer.extra},
            };
        }

        QJsonObject phonemeToJson(const SynthesisPhoneme &phoneme) {
            return {
                {QStringLiteral("token"), phoneme.token},
                {QStringLiteral("onset"), phoneme.onset},
                {QStringLiteral("language"), phoneme.language},
                {QStringLiteral("start"), phoneme.start},
            };
        }

        SynthesisPhoneme phonemeFromJson(const QJsonObject &object) {
            return {
                object.value(QStringLiteral("token")).toString(),
                object.value(QStringLiteral("onset")).toBool(),
                object.value(QStringLiteral("language")).toString(),
                object.value(QStringLiteral("start")).toDouble(),
            };
        }

        SynthesisParameter parameterFromJson(const QJsonObject &object) {
            return {
                doublesFromJson(object.value(QStringLiteral("values"))),
                object.value(QStringLiteral("sampleRate")).toDouble(100.0),
            };
        }

    }

    QString typeName(SynthesisTaskType type) {
        switch (type) {
            case SynthesisTaskType::Pronunciation: return QStringLiteral("pronunciation");
            case SynthesisTaskType::Phoneme: return QStringLiteral("phoneme");
            case SynthesisTaskType::Duration: return QStringLiteral("duration");
            case SynthesisTaskType::Parameter: return QStringLiteral("parameter");
            case SynthesisTaskType::Audio: return QStringLiteral("audio");
        }
        return QStringLiteral("unknown");
    }

    QJsonObject contextToJson(const SynthesisContext &context) {
        QJsonArray singers;
        for (const auto &singer : context.singers) {
            singers.append(singerToJson(singer));
        }
        return {
            {QStringLiteral("architecture"), context.architectureId},
            {QStringLiteral("architectureExtra"), context.architectureExtra},
            {QStringLiteral("singers"), singers},
        };
    }

    QJsonObject parameterToJson(const SynthesisParameter &parameter) {
        return {
            {QStringLiteral("values"), doublesToJson(parameter.values)},
            {QStringLiteral("sampleRate"), parameter.sampleRate},
        };
    }

    QJsonObject scoreCommonToJson(const SynthesisScore &score) {
        QJsonArray notes;
        for (const auto &note : score.notes) {
            QJsonArray phonemes;
            for (const auto &phoneme : note.phonemes) {
                phonemes.append(phonemeToJson(phoneme));
            }
            notes.append(QJsonObject{
                {QStringLiteral("gap"), note.gap},
                {QStringLiteral("duration"), note.duration},
                {QStringLiteral("cent"), note.cent},
                {QStringLiteral("pronunciation"), note.pronunciation},
                {QStringLiteral("language"), note.language},
                {QStringLiteral("phonemes"), phonemes},
            });
        }
        QJsonArray mix;
        for (const auto &row : score.mix) {
            mix.append(doublesToJson(row));
        }
        return {
            {QStringLiteral("pieceDuration"), score.pieceDuration},
            {QStringLiteral("notes"), notes},
            {QStringLiteral("mix"), mix},
            {QStringLiteral("mixSampleRate"), score.mixSampleRate},
        };
    }

    QJsonObject requestToJson(const SynthesisTaskRequest &request) {
        QJsonArray lyrics;
        for (const auto &note : request.lyricNotes) {
            lyrics.append(QJsonObject{
                {QStringLiteral("lyric"), note.lyric},
                {QStringLiteral("language"), note.language},
            });
        }
        QJsonArray pronunciations;
        for (const auto &note : request.pronunciationNotes) {
            pronunciations.append(QJsonObject{
                {QStringLiteral("pronunciation"), note.pronunciation},
                {QStringLiteral("language"), note.language},
            });
        }
        auto score = scoreCommonToJson(request.score);
        QJsonObject parameters;
        for (auto it = request.score.parameters.cbegin(); it != request.score.parameters.cend(); ++it) {
            parameters.insert(it.key(), parameterToJson(it.value()));
        }
        score.insert(QStringLiteral("parameters"), parameters);
        QJsonArray requested;
        for (const auto &id : request.score.requestedParameters) {
            requested.append(id);
        }
        score.insert(QStringLiteral("requestedParameters"), requested);
        return {
            {QStringLiteral("type"), typeName(request.type)},
            {QStringLiteral("context"), contextToJson(request.context)},
            {QStringLiteral("lyrics"), lyrics},
            {QStringLiteral("pronunciations"), pronunciations},
            {QStringLiteral("score"), score},
        };
    }

    QJsonObject resultToJson(const SynthesisTaskResult &result) {
        QJsonArray pronunciations;
        for (const auto &item : result.pronunciations) {
            QJsonArray candidates;
            for (const auto &candidate : item.candidates) {
                candidates.append(candidate);
            }
            pronunciations.append(QJsonObject{
                {QStringLiteral("pronunciation"), item.pronunciation},
                {QStringLiteral("candidates"), candidates},
            });
        }
        QJsonArray phonemeNotes;
        for (const auto &note : result.phonemes) {
            QJsonArray phonemes;
            for (const auto &phoneme : note) {
                phonemes.append(phonemeToJson(phoneme));
            }
            phonemeNotes.append(phonemes);
        }
        QJsonObject parameters;
        for (auto it = result.parameters.cbegin(); it != result.parameters.cend(); ++it) {
            parameters.insert(it.key(), parameterToJson(it.value()));
        }
        return {
            {QStringLiteral("pronunciations"), pronunciations},
            {QStringLiteral("phonemes"), phonemeNotes},
            {QStringLiteral("parameters"), parameters},
            {QStringLiteral("audioFilePath"), result.audioFilePath},
        };
    }

    bool resultFromJson(const QJsonObject &object, SynthesisTaskResult *result) {
        if (!result) {
            return false;
        }
        SynthesisTaskResult parsed;
        for (const auto &value : object.value(QStringLiteral("pronunciations")).toArray()) {
            const auto item = value.toObject();
            QStringList candidates;
            for (const auto &candidate : item.value(QStringLiteral("candidates")).toArray()) {
                candidates.append(candidate.toString());
            }
            parsed.pronunciations.append({
                item.value(QStringLiteral("pronunciation")).toString(),
                candidates,
            });
        }
        for (const auto &value : object.value(QStringLiteral("phonemes")).toArray()) {
            QList<SynthesisPhoneme> note;
            for (const auto &phoneme : value.toArray()) {
                note.append(phonemeFromJson(phoneme.toObject()));
            }
            parsed.phonemes.append(note);
        }
        const auto parameters = object.value(QStringLiteral("parameters")).toObject();
        for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it) {
            parsed.parameters.insert(it.key(), parameterFromJson(it.value().toObject()));
        }
        parsed.audioFilePath = object.value(QStringLiteral("audioFilePath")).toString();
        *result = std::move(parsed);
        return true;
    }

    QByteArray digest(const QJsonObject &object) {
        const auto data = QCborValue::fromJsonValue(object).toCbor();
        const auto result = XXH3_128bits(data.data(), data.size());
        return QByteArray(reinterpret_cast<const char *>(&result), sizeof(result)).toBase64(QByteArray::Base64UrlEncoding);
    }

    Api::V1::MultiSingerContext multiContext(const SynthesisContext &context) {
        Api::V1::MultiSingerContext result;
        result.arch = context.architectureId;
        result.archExtra = context.architectureExtra;
        for (const auto &singer : context.singers) {
            result.singers.append({singer.id, singer.extra});
        }
        return result;
    }

    Api::V1::SingleSingerContext singleContext(const SynthesisContext &context) {
        Api::V1::SingleSingerContext result;
        result.arch = context.architectureId;
        result.archExtra = context.architectureExtra;
        if (!context.singers.isEmpty()) {
            result.singer = {context.singers.first().id, context.singers.first().extra};
        }
        return result;
    }

    QList<Api::V1::ParameterNote> parameterNotes(const SynthesisScore &score) {
        QList<Api::V1::ParameterNote> result;
        for (const auto &note : score.notes) {
            Api::V1::ParameterNote converted;
            converted.position = {note.gap, note.duration};
            converted.cent = note.cent;
            converted.pronunciation = note.pronunciation;
            converted.language = note.language;
            for (const auto &phoneme : note.phonemes) {
                converted.phonemes.append({phoneme.token, phoneme.onset, phoneme.language, phoneme.start});
            }
            result.append(converted);
        }
        return result;
    }

    Api::V1::Mix mixToDto(const QList<QList<double>> &mix) {
        Api::V1::Mix result;
        result.rows = mix;
        return result;
    }

}
