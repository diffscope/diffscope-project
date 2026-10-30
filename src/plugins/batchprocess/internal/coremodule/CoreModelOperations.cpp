// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CoreModelOperations.h"

#include <cmath>
#include <memory>
#include <vector>

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QVariant>

#include <opendspx/serializer/jsonconverterv1.h>

namespace BatchProcess::Internal {
    namespace {
        stdc::json::Value nativeJson(const QJsonValue &value) {
            if (value.isNull() || value.isUndefined()) return {};
            if (value.isBool()) return value.toBool();
            if (value.isString()) return value.toString().toStdString();
            if (value.isDouble()) {
                const auto number = value.toDouble();
                if (number >= -2147483648.0 && number <= 2147483647.0 && std::floor(number) == number)
                    return static_cast<int>(number);
                return number;
            }
            if (value.isArray()) {
                stdc::json::Array array;
                for (const auto &item : value.toArray()) array.push_back(nativeJson(item));
                return array;
            }
            stdc::json::Object object;
            const auto source = value.toObject();
            for (auto it = source.begin(); it != source.end(); ++it)
                object[it.key().toStdString()] = nativeJson(it.value());
            return object;
        }

        bool qtJson(const stdc::json::Value &value, QJsonValue *result) {
            if (value.isNull()) { *result = QJsonValue(QJsonValue::Null); return true; }
            if (value.isBool()) { *result = value.toBool(); return true; }
            if (value.isString()) { *result = QString::fromStdString(value.toString()); return true; }
            if (value.isNumber()) {
                const auto number = value.toDouble();
                if (!std::isfinite(number)) return false;
                *result = number;
                return true;
            }
            if (value.isArray()) {
                QJsonArray array;
                for (const auto &item : value.toArray()) {
                    QJsonValue converted;
                    if (!qtJson(item, &converted)) return false;
                    array.append(converted);
                }
                *result = array;
                return true;
            }
            QJsonObject object;
            for (const auto &[key, item] : value.toObject()) {
                QJsonValue converted;
                if (!qtJson(item, &converted)) return false;
                object.insert(QString::fromStdString(key), converted);
            }
            *result = object;
            return true;
        }

        template <typename T>
        bool writeJson(const T &value, QJsonValue *data) {
            opendspx::SerializationErrorList errors;
            auto converted = opendspx::JsonConverterV1::toJson(value, errors, {opendspx::Serializer::CheckError});
            return !errors.containsError() && !errors.containsFatal() && qtJson(converted, data);
        }

        QObject *pointerProperty(QObject *object, const char *name) {
            return object ? object->property(name).value<QObject *>() : nullptr;
        }

        void destroyContents(QObject *container) {
            const auto items = modelItems(container);
            for (auto *object : items) if (auto *entity = qobject_cast<dspx::EntityObject *>(object); entity && liveEntity(entity)) entity->model()->destroyItem(entity);
        }

        void prepareReplacement(QObject *object) {
            destroyContents(object);
            if (auto *model = qobject_cast<dspx::Model *>(object)) {
                destroyContents(model->labels());
                destroyContents(model->keySignatures());
                destroyContents(model->tempos());
                destroyContents(model->timeSignatures());
                destroyContents(model->tracks());
                destroyContents(model->audioDSPs());
                model->setLoopEnabled(false);
                model->setLoopStart(0);
                model->setLoopLength(1920);
                model->setMultiChannelOutput(false);
            }
            if (auto *track = qobject_cast<dspx::Track *>(object)) {
                destroyContents(track->clips());
                destroyContents(track->audioDSPs());
                track->setColorId(0);
                track->setHeight(80.0);
            }
            if (auto *clip = qobject_cast<dspx::SingingClip *>(object)) {
                destroyContents(clip->notes());
                destroyContents(clip->parameters());
                if (auto *sources = clip->sources()) clip->model()->destroyItem(sources);
            }
            if (auto *note = qobject_cast<dspx::Note *>(object)) {
                destroyContents(note->originalPhonemes());
                destroyContents(note->editedPhonemes());
            }
            if (auto *parameter = qobject_cast<dspx::Parameter *>(object)) {
                destroyContents(parameter->anchorTransform());
                destroyContents(parameter->anchorEdited());
            }
            if (auto *sources = qobject_cast<dspx::Sources *>(object)) {
                destroyContents(sources->dynamicMixingAnchors());
                destroyContents(sources->singers());
            }
            if (auto *singer = qobject_cast<dspx::MixedSinger *>(object)) destroyContents(singer->singers());
        }
    }

    QString modelObjectKind(QObject *object) {
        if (qobject_cast<dspx::Track *>(object)) return QStringLiteral("Track");
        if (qobject_cast<dspx::TrackList *>(object)) return QStringLiteral("TrackList");
        if (qobject_cast<dspx::ClipSequence *>(object)) return QStringLiteral("ClipSequence");
        if (qobject_cast<dspx::AudioClip *>(object)) return QStringLiteral("AudioClip");
        if (qobject_cast<dspx::SingingClip *>(object)) return QStringLiteral("SingingClip");
        if (qobject_cast<dspx::Note *>(object)) return QStringLiteral("Note");
        if (qobject_cast<dspx::NoteSequence *>(object)) return QStringLiteral("NoteSequence");
        if (qobject_cast<dspx::Phoneme *>(object)) return QStringLiteral("Phoneme");
        if (qobject_cast<dspx::PhonemeSequence *>(object)) return QStringLiteral("PhonemeSequence");
        if (qobject_cast<dspx::Parameter *>(object)) return QStringLiteral("Parameter");
        if (qobject_cast<dspx::ParameterMap *>(object)) return QStringLiteral("ParameterMap");
        if (qobject_cast<dspx::AnchorNode *>(object)) return QStringLiteral("AnchorNode");
        if (qobject_cast<dspx::AnchorNodeSequence *>(object)) return QStringLiteral("AnchorNodeSequence");
        if (qobject_cast<dspx::Sources *>(object)) return QStringLiteral("Sources");
        if (qobject_cast<dspx::SingleSinger *>(object)) return QStringLiteral("SingleSinger");
        if (qobject_cast<dspx::MixedSinger *>(object)) return QStringLiteral("MixedSinger");
        if (qobject_cast<dspx::SingerList *>(object)) return QStringLiteral("SingerList");
        if (qobject_cast<dspx::DynamicMixingAnchor *>(object)) return QStringLiteral("DynamicMixingAnchor");
        if (qobject_cast<dspx::DynamicMixingAnchorSequence *>(object)) return QStringLiteral("DynamicMixingAnchorSequence");
        if (qobject_cast<dspx::Tempo *>(object)) return QStringLiteral("Tempo");
        if (qobject_cast<dspx::TempoSequence *>(object)) return QStringLiteral("TempoSequence");
        if (qobject_cast<dspx::TimeSignature *>(object)) return QStringLiteral("TimeSignature");
        if (qobject_cast<dspx::TimeSignatureSequence *>(object)) return QStringLiteral("TimeSignatureSequence");
        if (qobject_cast<dspx::Label *>(object)) return QStringLiteral("Label");
        if (qobject_cast<dspx::LabelSequence *>(object)) return QStringLiteral("LabelSequence");
        if (qobject_cast<dspx::KeySignature *>(object)) return QStringLiteral("KeySignature");
        if (qobject_cast<dspx::KeySignatureSequence *>(object)) return QStringLiteral("KeySignatureSequence");
        if (qobject_cast<dspx::AudioDSP *>(object)) return QStringLiteral("AudioDSP");
        if (qobject_cast<dspx::AudioDSPList *>(object)) return QStringLiteral("AudioDSPList");
        if (qobject_cast<dspx::FreeValueDataArray *>(object)) return QStringLiteral("FreeValueDataArray");
        if (qobject_cast<dspx::VibratoPointDataArray *>(object)) return QStringLiteral("VibratoPointDataArray");
        if (qobject_cast<dspx::Model *>(object)) return QStringLiteral("DspxModel");
        return {};
    }

    QObject *modelOwner(QObject *object) {
        if (!object || qobject_cast<dspx::Model *>(object)) return nullptr;
        if (qobject_cast<dspx::EntityObject *>(object)) {
            static const char *const parents[] = {"trackList", "clipSequence", "noteSequence", "phonemeSequence",
                "parameterMap", "anchorNodeSequence", "singingClip", "singerList", "dynamicMixingAnchorSequence",
                "tempoSequence", "timeSignatureSequence", "labelSequence", "keySignatureSequence", "audioDSPList"};
            for (const auto *name : parents) if (auto *parent = pointerProperty(object, name)) return parent;
            return nullptr;
        }
        static const char *const parents[] = {"singingClip", "sources", "mixedSinger", "track", "note", "parameter", "model"};
        for (const auto *name : parents) if (auto *parent = pointerProperty(object, name)) return parent;
        return object->parent();
    }

    dspx::Model *owningModel(QObject *object) {
        QSet<QObject *> visited;
        while (object && !visited.contains(object)) {
            if (auto *entity = qobject_cast<dspx::EntityObject *>(object)) return entity->model();
            visited.insert(object);
            object = modelOwner(object);
        }
        return nullptr;
    }

    bool liveEntity(dspx::EntityObject *entity) {
        return entity && entity->model() && entity->model()->find(*entity->metaObject(), entity->handle()) == entity;
    }

    bool attachedObject(QObject *object) {
        QSet<QObject *> visited;
        while (object && !visited.contains(object)) {
            if (qobject_cast<dspx::Model *>(object)) return true;
            if (auto *entity = qobject_cast<dspx::EntityObject *>(object); entity && !liveEntity(entity)) return false;
            visited.insert(object);
            object = modelOwner(object);
        }
        return false;
    }

    QList<QObject *> modelItems(QObject *object) {
        QList<QObject *> result;
        if (auto *container = qobject_cast<dspx::TrackList *>(object)) { for (auto *item : container->items()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::SingerList *>(object)) { for (auto *item : container->items()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::AudioDSPList *>(object)) { for (auto *item : container->items()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::ParameterMap *>(object)) { for (auto *item : container->items()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::ClipSequence *>(object)) { for (auto *item : container->asRange()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::NoteSequence *>(object)) { for (auto *item : container->asRange()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::PhonemeSequence *>(object)) { for (auto *item : container->asRange()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::AnchorNodeSequence *>(object)) { for (auto *item : container->asRange()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::DynamicMixingAnchorSequence *>(object)) { for (auto *item : container->asRange()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::TempoSequence *>(object)) { for (auto *item : container->asRange()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::TimeSignatureSequence *>(object)) { for (auto *item : container->asRange()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::LabelSequence *>(object)) { for (auto *item : container->asRange()) result.append(item); return result; }
        if (auto *container = qobject_cast<dspx::KeySignatureSequence *>(object)) { for (auto *item : container->asRange()) result.append(item); return result; }
        return result;
    }

    QList<QPointer<dspx::EntityObject>> modelEntities(dspx::Model *model) {
        QList<QPointer<dspx::EntityObject>> result;
        if (model) for (auto *item : model->findChildren<dspx::EntityObject *>(QString(), Qt::FindDirectChildrenOnly)) result.append(item);
        return result;
    }

    bool exportDSPX(QObject *object, QJsonValue *data) {
        if (auto *nodes = qobject_cast<dspx::AnchorNodeSequence *>(object)) {
            std::vector<opendspx::AnchorNode> values;
            for (auto *node : nodes->asRange()) values.push_back(node->toOpenDSPX());
            return writeJson(values, data);
        }
        if (auto *value = qobject_cast<dspx::Model *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::Track *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::TrackList *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::AudioClip *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::SingingClip *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::ClipSequence *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::Note *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::NoteSequence *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::Phoneme *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::PhonemeSequence *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::Parameter *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::ParameterMap *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::AnchorNode *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::Sources *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::SingleSinger *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::MixedSinger *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::SingerList *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::DynamicMixingAnchor *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::DynamicMixingAnchorSequence *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::Tempo *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::TempoSequence *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::TimeSignature *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::TimeSignatureSequence *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::Label *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::LabelSequence *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::VibratoPointDataArray *>(object)) return writeJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::KeySignature *>(object)) return qtJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::KeySignatureSequence *>(object)) return qtJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::AudioDSP *>(object)) return qtJson(value->toOpenDSPX(), data);
        if (auto *value = qobject_cast<dspx::AudioDSPList *>(object)) return qtJson(value->toOpenDSPX(), data);
        return false;
    }

    bool importDSPX(QObject *object, const QJsonValue &data, bool checked) {
        const auto source = nativeJson(data);
        const opendspx::Serializer::ConversionOption options = checked ? opendspx::Serializer::ConversionOption(opendspx::Serializer::CheckError) : opendspx::Serializer::ConversionOption{};
        if (auto *nodes = qobject_cast<dspx::AnchorNodeSequence *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto values = opendspx::JsonConverterV1::fromJson<std::vector<opendspx::AnchorNode>>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            destroyContents(nodes);
            for (const auto &value : values) {
                auto *node = nodes->parameter()->model()->createAnchorNode();
                node->fromOpenDSPX(value);
                if (!nodes->insertItem(node)) { node->model()->destroyItem(node); return false; }
            }
            return true;
        }
        if (auto *value = qobject_cast<dspx::Model *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::Model>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::Track *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::Track>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::TrackList *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<std::vector<opendspx::Track>>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::AudioClip *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::AudioClip>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::SingingClip *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::SingingClip>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::ClipSequence *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<std::vector<std::shared_ptr<opendspx::Clip>>>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::Note *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::Note>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::NoteSequence *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<std::vector<opendspx::Note>>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::Phoneme *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::Phoneme>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::PhonemeSequence *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<std::vector<opendspx::Phoneme>>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::Parameter *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::Param>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::ParameterMap *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::Params>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::AnchorNode *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::AnchorNode>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::Sources *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::Sources>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::SingleSinger *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::SingleSinger>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::MixedSinger *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::MixedSinger>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::SingerList *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<std::vector<std::shared_ptr<opendspx::Singer>>>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::DynamicMixingAnchor *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::DynamicMixingAnchor>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::DynamicMixingAnchorSequence *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<std::vector<opendspx::DynamicMixingAnchor>>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::Tempo *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::Tempo>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::TempoSequence *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<std::vector<opendspx::Tempo>>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::TimeSignature *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::TimeSignature>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::TimeSignatureSequence *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<std::vector<opendspx::TimeSignature>>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::Label *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<opendspx::Label>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::LabelSequence *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<std::vector<opendspx::Label>>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::VibratoPointDataArray *>(object)) {
            opendspx::SerializationErrorList errors;
            const auto converted = opendspx::JsonConverterV1::fromJson<std::vector<opendspx::ControlPoint>>(source, errors, options);
            if (errors.containsFatal() || (checked && errors.containsError())) return false;
            prepareReplacement(value);
            value->fromOpenDSPX(converted);
            return true;
        }
        if (auto *value = qobject_cast<dspx::KeySignature *>(object)) { value->fromOpenDSPX(source); return true; }
        if (auto *value = qobject_cast<dspx::KeySignatureSequence *>(object)) { prepareReplacement(value); value->fromOpenDSPX(source); return true; }
        if (auto *value = qobject_cast<dspx::AudioDSP *>(object)) { value->fromOpenDSPX(source.toObject()); return true; }
        if (auto *value = qobject_cast<dspx::AudioDSPList *>(object)) { prepareReplacement(value); value->fromOpenDSPX(source.toArray()); return true; }
        return false;
    }

    dspx::EntityObject *allocateEntity(dspx::Model *model, const QString &kind, bool original) {
        if (!model) return nullptr;
        if (kind == QStringLiteral("Phoneme") && original) return model->createOriginalPhoneme();
        if (kind == QStringLiteral("Track")) return model->createTrack();
        if (kind == QStringLiteral("AudioClip")) return model->createAudioClip();
        if (kind == QStringLiteral("SingingClip")) return model->createSingingClip();
        if (kind == QStringLiteral("Note")) return model->createNote();
        if (kind == QStringLiteral("Phoneme")) return model->createPhoneme();
        if (kind == QStringLiteral("Parameter")) return model->createParameter();
        if (kind == QStringLiteral("AnchorNode")) return model->createAnchorNode();
        if (kind == QStringLiteral("Sources")) return model->createSources();
        if (kind == QStringLiteral("SingleSinger")) return model->createSingleSinger();
        if (kind == QStringLiteral("MixedSinger")) return model->createMixedSinger();
        if (kind == QStringLiteral("DynamicMixingAnchor")) return model->createDynamicMixingAnchor();
        if (kind == QStringLiteral("Tempo")) return model->createTempo();
        if (kind == QStringLiteral("TimeSignature")) return model->createTimeSignature();
        if (kind == QStringLiteral("Label")) return model->createLabel();
        if (kind == QStringLiteral("KeySignature")) return model->createKeySignature();
        if (kind == QStringLiteral("AudioDSP")) return model->createAudioDSP();
        return nullptr;
    }

    bool attachEntity(QObject *container, dspx::EntityObject *entity, int index, const QString &key) {
        if (!container || !entity || owningModel(container) != entity->model()) return false;
        if (auto *value = qobject_cast<dspx::TrackList *>(container)) { auto *item = qobject_cast<dspx::Track *>(entity); return item && value->insertItem(index, item); }
        if (auto *value = qobject_cast<dspx::SingerList *>(container)) { auto *item = qobject_cast<dspx::Singer *>(entity); return item && value->insertItem(index, item); }
        if (auto *value = qobject_cast<dspx::AudioDSPList *>(container)) { auto *item = qobject_cast<dspx::AudioDSP *>(entity); return item && value->insertItem(index, item); }
        if (auto *value = qobject_cast<dspx::ClipSequence *>(container)) { auto *item = qobject_cast<dspx::Clip *>(entity); return item && value->insertItem(item); }
        if (auto *value = qobject_cast<dspx::NoteSequence *>(container)) { auto *item = qobject_cast<dspx::Note *>(entity); return item && value->insertItem(item); }
        if (auto *value = qobject_cast<dspx::PhonemeSequence *>(container)) { auto *item = qobject_cast<dspx::Phoneme *>(entity); return item && value->insertItem(item); }
        if (auto *value = qobject_cast<dspx::AnchorNodeSequence *>(container)) { auto *item = qobject_cast<dspx::AnchorNode *>(entity); return item && value->insertItem(item); }
        if (auto *value = qobject_cast<dspx::DynamicMixingAnchorSequence *>(container)) { auto *item = qobject_cast<dspx::DynamicMixingAnchor *>(entity); return item && value->insertItem(item); }
        if (auto *value = qobject_cast<dspx::TempoSequence *>(container)) { auto *item = qobject_cast<dspx::Tempo *>(entity); return item && value->insertItem(item); }
        if (auto *value = qobject_cast<dspx::TimeSignatureSequence *>(container)) { auto *item = qobject_cast<dspx::TimeSignature *>(entity); return item && value->insertItem(item); }
        if (auto *value = qobject_cast<dspx::LabelSequence *>(container)) { auto *item = qobject_cast<dspx::Label *>(entity); return item && value->insertItem(item); }
        if (auto *value = qobject_cast<dspx::KeySignatureSequence *>(container)) { auto *item = qobject_cast<dspx::KeySignature *>(entity); return item && value->insertItem(item); }
        if (auto *value = qobject_cast<dspx::ParameterMap *>(container)) { auto *item = qobject_cast<dspx::Parameter *>(entity); return item && value->insertItem(key, item); }
        if (auto *value = qobject_cast<dspx::SingingClip *>(container)) { auto *item = qobject_cast<dspx::Sources *>(entity); if (!item) return false; value->setSources(item); return true; }
        return false;
    }

    bool transferEntity(QObject *container, dspx::EntityObject *entity, int index) {
        auto *owner = modelOwner(entity);
        if (!owner || owner == container || owningModel(container) != entity->model()) return false;
        if (auto *value = qobject_cast<dspx::ClipSequence *>(container)) { auto *source = qobject_cast<dspx::ClipSequence *>(owner); auto *item = qobject_cast<dspx::Clip *>(entity); return source && item && source->moveItem(item, value); }
        if (auto *value = qobject_cast<dspx::NoteSequence *>(container)) { auto *source = qobject_cast<dspx::NoteSequence *>(owner); auto *item = qobject_cast<dspx::Note *>(entity); return source && item && source->moveItem(item, value); }
        if (auto *value = qobject_cast<dspx::AnchorNodeSequence *>(container)) { auto *source = qobject_cast<dspx::AnchorNodeSequence *>(owner); auto *item = qobject_cast<dspx::AnchorNode *>(entity); return source && item && source->moveItem(item, value); }
        if (auto *value = qobject_cast<dspx::DynamicMixingAnchorSequence *>(container)) { auto *source = qobject_cast<dspx::DynamicMixingAnchorSequence *>(owner); auto *item = qobject_cast<dspx::DynamicMixingAnchor *>(entity); return source && item && source->moveItem(item, value); }
        const auto sourceIndex = modelItems(owner).indexOf(entity);
        if (auto *value = qobject_cast<dspx::SingerList *>(container)) {
            auto *source = qobject_cast<dspx::SingerList *>(owner);
            return source && source->moveItem(static_cast<int>(sourceIndex), value, index);
        }
        if (auto *source = qobject_cast<dspx::AudioDSPList *>(owner)) {
            auto *value = qobject_cast<dspx::AudioDSPList *>(container);
            if (!value) return false;
            auto *item = source->removeItem(static_cast<int>(sourceIndex));
            try {
                if (value->insertItem(index, item)) return true;
            } catch (...) {
                source->insertItem(static_cast<int>(sourceIndex), item);
                throw;
            }
            source->insertItem(static_cast<int>(sourceIndex), item);
            return false;
        }
        return false;
    }

    bool rotateContainer(QObject *container, int leftIndex, int middleIndex, int rightIndex) {
        if (!container || leftIndex < 0 || middleIndex < leftIndex || rightIndex < middleIndex || rightIndex > container->property("size").toInt()) return false;
        const auto rotate = [=](auto *value) { return leftIndex == middleIndex || middleIndex == rightIndex || value->rotate(leftIndex, middleIndex, rightIndex); };
        if (auto *value = qobject_cast<dspx::TrackList *>(container)) return rotate(value);
        if (auto *value = qobject_cast<dspx::SingerList *>(container)) return rotate(value);
        if (auto *value = qobject_cast<dspx::AudioDSPList *>(container)) return rotate(value);
        if (auto *value = qobject_cast<dspx::FreeValueDataArray *>(container)) return value->role() != dspx::FreeValueDataArray::Original && rotate(value);
        if (auto *value = qobject_cast<dspx::VibratoPointDataArray *>(container)) return rotate(value);
        return false;
    }
}

