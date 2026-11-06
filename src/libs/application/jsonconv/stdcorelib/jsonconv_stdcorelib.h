// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef JSONCONV_STDCORELIB_H
#define JSONCONV_STDCORELIB_H

#include <cmath>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <stdcorelib/support/json.h>

namespace jsonconv_stdcorelib {

    enum class IntegerConversion {
        ToDouble,
        PreserveInt32,
    };

    struct ToQJsonValueOptions {
        bool rejectNonFinite = false;
        bool unsupportedTypeAsEmptyObject = false;
    };

    inline stdc::json::Value fromQJsonValue(const QJsonValue &value, IntegerConversion integers = IntegerConversion::ToDouble) {
        if (value.isString()) {
            return value.toString().toStdString();
        }
        if (value.isBool()) {
            return value.toBool();
        }
        if (value.isDouble()) {
            const auto number = value.toDouble();
            if (integers == IntegerConversion::PreserveInt32) {
                const auto integer = value.toInt();
                if (number == integer) {
                    return integer;
                }
            }
            return number;
        }
        if (value.isArray()) {
            stdc::json::Array result;
            for (const auto &item : value.toArray()) {
                result.push_back(fromQJsonValue(item, integers));
            }
            return result;
        }
        if (value.isObject()) {
            stdc::json::Object result;
            for (auto [key, item] : value.toObject().asKeyValueRange()) {
                result[key.toString().toStdString()] = fromQJsonValue(item, integers);
            }
            return result;
        }
        return {};
    }

    inline QJsonValue toQJsonValue(const stdc::json::Value &value, ToQJsonValueOptions options = {}) {
        if (value.isString()) {
            return QString::fromStdString(value.toString());
        }
        if (value.isBool()) {
            return value.toBool();
        }
        if (value.isNumber()) {
            const auto number = value.toDouble();
            if (options.rejectNonFinite && !std::isfinite(number)) {
                return QJsonValue(QJsonValue::Undefined);
            }
            return number;
        }
        if (value.isArray()) {
            QJsonArray result;
            for (const auto &item : value.toArray()) {
                const auto converted = toQJsonValue(item, options);
                if (converted.isUndefined()) {
                    return converted;
                }
                result.append(converted);
            }
            return result;
        }
        if (value.isObject()) {
            QJsonObject result;
            for (const auto &[key, item] : value.toObject()) {
                const auto converted = toQJsonValue(item, options);
                if (converted.isUndefined()) {
                    return converted;
                }
                result.insert(QString::fromStdString(key), converted);
            }
            return result;
        }
        if (options.unsupportedTypeAsEmptyObject && !value.isNull()) {
            return QJsonObject{};
        }
        return {};
    }

}

#endif // JSONCONV_STDCORELIB_H
