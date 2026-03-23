#ifndef DIFFSCOPE_DSPXMODEL_JSONUTILS_P_H
#define DIFFSCOPE_DSPXMODEL_JSONUTILS_P_H

#include <algorithm>

#include <nlohmann/json.hpp>

#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>

namespace dspx {

    class JsonUtils {
    public:
        static nlohmann::json fromQJsonValue(const QJsonValue &v) {
            if(v.isString())
                return v.toString().toStdString();

            if(v.isBool())
                return v.toBool();

            if(v.isDouble())
                return v.toDouble();

            if(v.isArray()) {
                nlohmann::json ret = nlohmann::json::array();
                std::ranges::transform(v.toArray(), std::back_inserter(ret), &JsonUtils::fromQJsonValue);
                return ret;
            }

            if(v.isObject()) {
                nlohmann::json ret = nlohmann::json::object();
                for (auto [key, value] : v.toObject().asKeyValueRange()) {
                    ret[key.toString().toStdString()] = fromQJsonValue(value);
                }
                return ret;
            }

            return {};
        }


        static QJsonValue toQJsonValue(const nlohmann::json &v) {
            if(v.is_string())
                return QString::fromStdString(v.get<std::string>());

            if(v.is_boolean())
                return v.get<bool>();

            if(v.is_number())
                return v.get<double>();

            if(v.is_array()) {
                QJsonArray ret;
                std::ranges::transform(v, std::back_inserter(ret), &JsonUtils::toQJsonValue);
                return ret;
            }

            if(v.is_object()) {
                QJsonObject ret;
                for (auto it = v.begin(); it != v.end(); ++it) {
                    ret[it.key().c_str()] = toQJsonValue(it.value());
                }
            }

            return {};
        }
    };

}

#endif //DIFFSCOPE_DSPXMODEL_JSONUTILS_P_H
