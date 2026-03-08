#include "DspxClipboardData.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <opendspxserializer/serializer.h>
#include <opendspxserializer/jsonconverterv1.h>

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
            default:
                return {};
        }
    }

    DspxClipboardData::Type DspxClipboardData::typeFromMimeType(const QString &mimeType, bool *ok) {
        if (ok)
            *ok = true;
        if (mimeType == "application/x.diffscope.clipboard.tempo+json") {
            return Tempo;
        }
        if (mimeType == "application/x.diffscope.clipboard.label+json") {
            return Label;
        }
        if (mimeType == "application/x.diffscope.clipboard.keysignature+json") {
            return KeySignature;
        }
        if (mimeType == "application/x.diffscope.clipboard.track+json") {
            return Track;
        }
        if (mimeType == "application/x.diffscope.clipboard.clip+json") {
            return Clip;
        }
        if (mimeType == "application/x.diffscope.clipboard.note+json") {
            return Note;
        }
        if (ok)
            *ok = false;
        return {};
    }

    QByteArray DspxClipboardData::toData() const {
        QJsonObject json;
        json.insert("version", QDspx::Serializer::versionToText(QDspx::Model::V1));
        json.insert("playhead", m_playhead);
        json.insert("absolute", m_absolute);
        json.insert("track", m_track);
        QJsonArray dataArray;
        QDspx::SerializationErrorList errors;
        auto toJsonArray = [&]<Type t> {
            for (const auto &item : std::get<t>(m_data)) {
                dataArray.append(QDspx::JsonConverterV1::toJson(item, errors, {}));
            }
        };
        switch (type()) {
            case Tempo:
                toJsonArray.operator()<Tempo>();
                break;
            case Label:
                toJsonArray.operator()<Label>();
                break;
            case KeySignature:
                for (const auto &item : std::get<KeySignature>(m_data)) {
                    dataArray.append(item);
                }
                break;
            case Track:
                toJsonArray.operator()<Track>();
                break;
            case Clip:
                toJsonArray.operator()<Clip>();
                break;
            case Note:
                toJsonArray.operator()<Note>();
                break;
        }
        json.insert("data", dataArray);
        return QJsonDocument(json).toJson(QJsonDocument::Compact);
    }

    DspxClipboardData DspxClipboardData::fromData(const QByteArray &data, Type type, bool *ok) {
        QJsonParseError jsonError;
        auto json = QJsonDocument::fromJson(data, &jsonError).object();
        if (jsonError.error != QJsonParseError::NoError) {
            if (ok)
                *ok = false;
            return {};
        }
        auto version = QDspx::Serializer::versionFromText(json.value("version").toString(), ok);
        if (ok && !*ok) {
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
        QDspx::SerializationErrorList errors;
        switch (version) {
            case QDspx::Model::V1: {
                auto fromJsonArrayV1 = [&]<Type t> {
                    using T = std::variant_alternative_t<t, decltype(m_data)>;
                    T list;
                    for (const auto &item : dataArray) {
                        list.append(QDspx::JsonConverterV1::fromJson<typename T::value_type>(item, errors));
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
                    case KeySignature: {
                        QList<QJsonObject> list;
                        for (const auto &value : dataArray) {
                            list.append(value.toObject());
                        }
                        result.m_data = std::move(list);
                        break;
                    }
                    case Track:
                        fromJsonArrayV1.operator()<Track>();
                        break;
                    case Clip:
                        fromJsonArrayV1.operator()<Clip>();
                        break;
                    case Note:
                        fromJsonArrayV1.operator()<Note>();
                        break;
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