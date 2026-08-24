// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EffectsPresets.h"

#include <algorithm>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>
#include <QVariantMap>

#include <CoreApi/runtimeinterface.h>

namespace Audio::Internal {

    namespace {

        QJsonValue toQJsonValue(const stdc::JsonValue &value) {
            if (value.isString()) {
                return QString::fromStdString(value.toString());
            }
            if (value.isBool()) {
                return value.toBool();
            }
            if (value.isNumber()) {
                return value.toDouble();
            }
            if (value.isArray()) {
                QJsonArray result;
                for (const auto &item : value.toArray()) {
                    result.append(toQJsonValue(item));
                }
                return result;
            }
            if (value.isObject()) {
                QJsonObject result;
                for (const auto &[key, item] : value.toObject()) {
                    result.insert(QString::fromStdString(key), toQJsonValue(item));
                }
                return result;
            }
            return {};
        }

        stdc::JsonValue fromQJsonValue(const QJsonValue &value) {
            switch (value.type()) {
                case QJsonValue::Null:
                case QJsonValue::Undefined:
                    return {};
                case QJsonValue::Bool:
                    return value.toBool();
                case QJsonValue::Double:
                    return value.toDouble();
                case QJsonValue::String:
                    return value.toString().toStdString();
                case QJsonValue::Array: {
                    stdc::JsonArray result;
                    for (const auto &item : value.toArray()) {
                        result.push_back(fromQJsonValue(item));
                    }
                    return result;
                }
                case QJsonValue::Object: {
                    stdc::JsonObject result;
                    for (auto it = value.toObject().begin(); it != value.toObject().end(); ++it) {
                        result[it.key().toStdString()] = fromQJsonValue(it.value());
                    }
                    return result;
                }
            }
            return {};
        }

    }

    EffectsPresets *EffectsPresets::m_instance = nullptr;

    EffectsPresets *EffectsPresets::instance() {
        return m_instance;
    }

    EffectsPresets::EffectsPresets(QObject *parent)
        : QObject(parent) {
        Q_ASSERT(!m_instance);
        m_instance = this;
    }

    EffectsPresets::~EffectsPresets() {
        m_instance = nullptr;
    }

    QStringList EffectsPresets::presetNames() const {
        QStringList result;
        result.reserve(m_presets.size());
        for (const auto &preset : m_presets) {
            result.append(preset.name);
        }
        return result;
    }

    bool EffectsPresets::hasPreset(const QString &name) const {
        return std::ranges::any_of(m_presets, [&name](const Preset &preset) {
            return preset.name == name;
        });
    }

    bool EffectsPresets::savePreset(const QString &name, const stdc::JsonArray &audioDSPs) {
        const auto normalizedName = name.trimmed();
        if (normalizedName.isEmpty()) {
            return false;
        }
        const auto data = serialize(audioDSPs);
        for (auto &preset : m_presets) {
            if (preset.name == normalizedName) {
                if (preset.data == data) {
                    return true;
                }
                preset.data = data;
                save();
                Q_EMIT presetsChanged();
                return true;
            }
        }
        m_presets.append({normalizedName, data});
        save();
        Q_EMIT presetsChanged();
        return true;
    }

    bool EffectsPresets::removePreset(const QString &name) {
        const auto it = std::ranges::find_if(m_presets, [&name](const Preset &preset) {
            return preset.name == name;
        });
        if (it == m_presets.end()) {
            return false;
        }
        m_presets.erase(it);
        save();
        Q_EMIT presetsChanged();
        return true;
    }

    stdc::JsonArray EffectsPresets::presetAudioDSPs(const QString &name) const {
        for (const auto &preset : m_presets) {
            if (preset.name == name) {
                return deserialize(preset.data);
            }
        }
        return {};
    }

    void EffectsPresets::load() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());

        m_presets.clear();
        const auto presets = settings->value(QStringLiteral("presets")).toList();
        for (const auto &value : presets) {
            const auto data = value.toMap();
            const auto name = data.value(QStringLiteral("name")).toString().trimmed();
            if (name.isEmpty() || hasPreset(name)) {
                continue;
            }
            m_presets.append({name, data.value(QStringLiteral("data")).toByteArray()});
        }

        settings->endGroup();
    }

    void EffectsPresets::save() const {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());

        QVariantList presets;
        presets.reserve(m_presets.size());
        for (const auto &preset : m_presets) {
            presets.append(QVariantMap{
                {QStringLiteral("name"), preset.name},
                {QStringLiteral("data"), preset.data},
            });
        }
        settings->setValue(QStringLiteral("presets"), presets);

        settings->endGroup();
    }

    QByteArray EffectsPresets::serialize(const stdc::JsonArray &audioDSPs) const {
        const auto json = toQJsonValue(audioDSPs).toArray();
        return QJsonDocument(json).toJson(QJsonDocument::Compact);
    }

    stdc::JsonArray EffectsPresets::deserialize(const QByteArray &data) const {
        const auto document = QJsonDocument::fromJson(data);
        if (!document.isArray()) {
            return {};
        }
        return fromQJsonValue(document.array()).toArray();
    }

}

#include "moc_EffectsPresets.cpp"
