// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DspxClipboardData.h"

#include <limits>
#include <type_traits>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <opendspx/serializer/jsonconverterv1.h>
#include <opendspx/serializer/serializer.h>

#include <dspxmodelORM/FreeValueDataArray.h>

#include <coreplugin/internal/jsonutils.h>

namespace Core {

    QString DspxClipboardData::mimeType(Type type) {
        switch (type) {
            case Tempo:
                return "application/x.diffscope.clipboard.tempo+json";
            case Label:
                return "application/x.diffscope.clipboard.label+json";
            case KeySignature:
                return "application/x.diffscope.clipboard.keysignature+json";
            case Track:
                return "application/x.diffscope.clipboard.track+json";
            case Clip:
                return "application/x.diffscope.clipboard.clip+json";
            case Note:
                return "application/x.diffscope.clipboard.note+json";
            case DynamicMixingAnchor:
                return "application/x.diffscope.clipboard.dynamicmixinganchor+json";
            case AnchorNode:
                return "application/x.diffscope.clipboard.anchornode+json";
            case FreeParameter:
                return "application/x.diffscope.clipboard.freeparameter+json";
            default:
                return {};
        }
    }

    DspxClipboardData::Type DspxClipboardData::typeFromMimeType(const QString &mimeType, bool *ok) {
        for (int index = Tempo; index <= FreeParameter; ++index) {
            const auto type = static_cast<Type>(index);
            if (mimeType == DspxClipboardData::mimeType(type)) {
                if (ok)
                    *ok = true;
                return type;
            }
        }
        if (ok)
            *ok = false;
        return {};
    }

    QByteArray DspxClipboardData::toData() const {
        QJsonObject json;
        json.insert("version", QString::fromStdString(opendspx::Serializer::versionToText(opendspx::Model::Version::V1)));
        json.insert("playhead", m_playhead);
        json.insert("absolute", m_absolute);
        json.insert("track", m_track);
        QJsonArray dataArray;
        opendspx::SerializationErrorList errors;
        std::visit([&](const auto &payload) {
            if constexpr (std::is_same_v<std::decay_t<decltype(payload)>, FreeParameterClipboardData>) {
                QJsonArray values;
                for (const auto &value : payload.values)
                    values.append(value.isValid() ? QJsonValue(value.toInt()) : QJsonValue(QJsonValue::Null));
                dataArray.append(QJsonObject {
                    {"length", payload.length},
                    {"offset", payload.offset},
                    {"values", values},
                });
            } else {
                for (const auto &item : payload)
                    dataArray.append(Internal::JsonUtils::toQJsonValue(opendspx::JsonConverterV1::toJson(item, errors, {})));
            }
        }, m_data);
        json.insert("data", dataArray);
        return QJsonDocument(json).toJson(QJsonDocument::Compact);
    }

    DspxClipboardData DspxClipboardData::fromData(const QByteArray &data, Type type, bool *ok) {
        if (ok)
            *ok = false;
        QJsonParseError jsonError;
        const auto document = QJsonDocument::fromJson(data, &jsonError);
        if (jsonError.error != QJsonParseError::NoError || !document.isObject()) {
            return {};
        }
        const auto json = document.object();
        bool versionOk = false;
        const auto version = opendspx::Serializer::versionFromText(json.value("version").toString().toStdString(), &versionOk);
        if (!versionOk) {
            return {};
        }
        DspxClipboardData result;
        result.m_playhead = json.value("playhead").toInt();
        result.m_absolute = json.value("absolute").toInt();
        result.m_track = json.value("track").toInt();
        if (!json.value("data").isArray()) {
            if (ok)
                *ok = false;
            return {};
        }
        auto dataArray = json.value("data").toArray();
        opendspx::SerializationErrorList errors;
        switch (version) {
            case opendspx::Model::Version::V1: {
                auto fromJsonArrayV1 = [&]<Type t> {
                    using T = std::variant_alternative_t<t, decltype(m_data)>;
                    T list;
                    for (const auto &item : dataArray) {
                        list.append(opendspx::JsonConverterV1::fromJson<typename T::value_type>(Internal::JsonUtils::fromQJsonValue(item), errors));
                    }
                    result.m_data = std::move(list);
                };
                switch (type) {
                    case Tempo:
                        fromJsonArrayV1.operator()<Tempo>();
                        break;
                    case Label:
                        fromJsonArrayV1.operator()<Label>();
                        break;
                    case KeySignature:
                        fromJsonArrayV1.operator()<KeySignature>();
                        break;
                    case Track:
                        fromJsonArrayV1.operator()<Track>();
                        break;
                    case Clip:
                        fromJsonArrayV1.operator()<Clip>();
                        break;
                    case Note:
                        fromJsonArrayV1.operator()<Note>();
                        break;
                    case DynamicMixingAnchor:
                        fromJsonArrayV1.operator()<DynamicMixingAnchor>();
                        break;
                    case AnchorNode:
                        fromJsonArrayV1.operator()<AnchorNode>();
                        break;
                    case FreeParameter: {
                        if (dataArray.size() != 1 || !dataArray.first().isObject())
                            return {};
                        const auto object = dataArray.first().toObject();
                        FreeParameterClipboardData parameter;
                        parameter.length = object.value("length").toInt(-1);
                        parameter.offset = object.value("offset").toInt(1);
                        const int step = dspx::FreeValueDataArray::step();
                        if (parameter.length <= 0 || parameter.length > std::numeric_limits<int>::max() - 2 * step ||
                            parameter.offset <= -step || parameter.offset > 0 || !object.value("values").isArray()) {
                            return {};
                        }
                        const auto values = object.value("values").toArray();
                        if (values.size() > (parameter.length - parameter.offset + step - 1) / step + 1)
                            return {};
                        parameter.values.reserve(values.size());
                        for (const auto &value : values) {
                            if (value.isNull()) {
                                parameter.values.append(QVariant());
                            } else if (value.isDouble() && value.toDouble() == value.toInt()) {
                                parameter.values.append(value.toInt());
                            } else {
                                return {};
                            }
                        }
                        result.m_data = std::move(parameter);
                        break;
                    }
                    default:
                        return {};
                }
                break;
            }

        }
        if (errors.containsFatal() || errors.containsError()) {
            if (ok)
                *ok = false;
            return {};
        }
        if (ok)
            *ok = true;
        return result;
    }

}
