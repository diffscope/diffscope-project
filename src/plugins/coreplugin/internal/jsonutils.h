// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_JSONUTILS_H
#define DIFFSCOPE_COREPLUGIN_JSONUTILS_H

#include <algorithm>

#include <stdcorelib/support/json.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace Core::Internal {

    class JsonUtils {
    public:
        static stdc::JsonValue fromQJsonValue(const QJsonValue &value) {
            if (value.isString())
                return value.toString().toStdString();

            if (value.isBool())
                return value.toBool();

            if (value.isDouble())
                return value.toDouble();

            if (value.isArray()) {
                stdc::JsonArray ret;
                std::ranges::transform(value.toArray(), std::back_inserter(ret), &JsonUtils::fromQJsonValue);
                return ret;
            }

            if (value.isObject()) {
                stdc::JsonObject ret;
                for (auto [key, item] : value.toObject().asKeyValueRange()) {
                    ret[key.toString().toStdString()] = fromQJsonValue(item);
                }
                return ret;
            }

            return {};
        }

        static QJsonValue toQJsonValue(const stdc::JsonValue &value) {
            if (value.isString())
                return QString::fromStdString(value.toString());

            if (value.isBool())
                return value.toBool();

            if (value.isNumber())
                return value.toDouble();

            if (value.isArray()) {
                QJsonArray ret;
                std::ranges::transform(value.toArray(), std::back_inserter(ret), &JsonUtils::toQJsonValue);
                return ret;
            }

            if (value.isObject()) {
                QJsonObject ret;
                for (const auto &[key, item] : value.toObject()) {
                    ret[QString::fromStdString(key)] = toQJsonValue(item);
                }
                return ret;
            }

            return {};
        }
    };

}

#endif // DIFFSCOPE_COREPLUGIN_JSONUTILS_H
