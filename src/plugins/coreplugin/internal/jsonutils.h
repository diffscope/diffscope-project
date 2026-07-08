#ifndef DIFFSCOPE_COREPLUGIN_JSONUTILS_H
#define DIFFSCOPE_COREPLUGIN_JSONUTILS_H

#include <algorithm>

#include <nlohmann/json.hpp>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace Core::Internal {

    class JsonUtils {
    public:
        static nlohmann::json fromQJsonValue(const QJsonValue &value) {
            if (value.isString())
                return value.toString().toStdString();

            if (value.isBool())
                return value.toBool();

            if (value.isDouble())
                return value.toDouble();

            if (value.isArray()) {
                nlohmann::json ret = nlohmann::json::array();
                std::ranges::transform(value.toArray(), std::back_inserter(ret), &JsonUtils::fromQJsonValue);
                return ret;
            }

            if (value.isObject()) {
                nlohmann::json ret = nlohmann::json::object();
                for (auto [key, item] : value.toObject().asKeyValueRange()) {
                    ret[key.toString().toStdString()] = fromQJsonValue(item);
                }
                return ret;
            }

            return {};
        }

        static QJsonValue toQJsonValue(const nlohmann::json &value) {
            if (value.is_string())
                return QString::fromStdString(value.get<std::string>());

            if (value.is_boolean())
                return value.get<bool>();

            if (value.is_number())
                return value.get<double>();

            if (value.is_array()) {
                QJsonArray ret;
                std::ranges::transform(value, std::back_inserter(ret), &JsonUtils::toQJsonValue);
                return ret;
            }

            if (value.is_object()) {
                QJsonObject ret;
                for (auto it = value.begin(); it != value.end(); ++it) {
                    ret[it.key().c_str()] = toQJsonValue(it.value());
                }
                return ret;
            }

            return {};
        }
    };

}

#endif // DIFFSCOPE_COREPLUGIN_JSONUTILS_H
