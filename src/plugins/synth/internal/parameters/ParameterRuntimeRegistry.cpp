// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ParameterRuntimeRegistry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <utility>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>

#include <xxhash.h>

namespace Synth::Internal {

    namespace {

        bool validateExpression(const QJsonValue &expression, int depth = 0) {
            if (depth > 64)
                return false;
            if (expression.isDouble())
                return std::isfinite(expression.toDouble());
            if (!expression.isObject())
                return false;
            const auto object = expression.toObject();
            if (object.size() != 2 || !object.value(QStringLiteral("operator")).isString()
                || !object.value(QStringLiteral("operands")).isArray())
                return false;
            const auto op = object.value(QStringLiteral("operator")).toString();
            const auto operands = object.value(QStringLiteral("operands")).toArray();
            static const QStringList constants{QStringLiteral("x"), QStringLiteral("e"), QStringLiteral("pi")};
            static const QStringList variadic{QStringLiteral("+"), QStringLiteral("*"), QStringLiteral("hypot"), QStringLiteral("min"), QStringLiteral("max")};
            static const QStringList binary{QStringLiteral("-"), QStringLiteral("/"), QStringLiteral("%"), QStringLiteral("pow"), QStringLiteral(">"), QStringLiteral("<"), QStringLiteral(">="), QStringLiteral("<="), QStringLiteral("=="), QStringLiteral("!="), QStringLiteral("||"), QStringLiteral("&&")};
            static const QStringList unary{
                QStringLiteral("abs"), QStringLiteral("acos"), QStringLiteral("acosh"), QStringLiteral("asin"), QStringLiteral("asinh"), QStringLiteral("atan"), QStringLiteral("atanh"),
                QStringLiteral("ceil"), QStringLiteral("cos"), QStringLiteral("cosh"), QStringLiteral("exp"), QStringLiteral("floor"), QStringLiteral("ln"), QStringLiteral("log"),
                QStringLiteral("round"), QStringLiteral("sign"), QStringLiteral("sin"), QStringLiteral("sinh"), QStringLiteral("sqrt"), QStringLiteral("tan"), QStringLiteral("tanh"), QStringLiteral("trunc"), QStringLiteral("!")};
            const bool validArity = constants.contains(op) ? operands.isEmpty()
                : variadic.contains(op) ? !operands.isEmpty()
                : binary.contains(op) ? operands.size() == 2
                : unary.contains(op) ? operands.size() == 1
                : op == QStringLiteral("if") && operands.size() == 3;
            if (!validArity)
                return false;
            for (const auto &operand : operands) {
                if (!validateExpression(operand, depth + 1))
                    return false;
            }
            return true;
        }

        double evaluateExpression(const QJsonValue &expression, double x) {
            if (expression.isDouble())
                return expression.toDouble();
            const auto object = expression.toObject();
            const auto op = object.value(QStringLiteral("operator")).toString();
            const auto operands = object.value(QStringLiteral("operands")).toArray();
            if (op == QStringLiteral("x"))
                return x;
            if (op == QStringLiteral("e"))
                return std::numbers::e;
            if (op == QStringLiteral("pi"))
                return std::numbers::pi;
            const double a = evaluateExpression(operands.at(0), x);
            if (!std::isfinite(a))
                return a;
            if (op == QStringLiteral("if"))
                return evaluateExpression(operands.at(a != 0 ? 1 : 2), x);
            if (op == QStringLiteral("||") && a != 0)
                return 1;
            if (op == QStringLiteral("&&") && a == 0)
                return 0;

            if (op == QStringLiteral("+") || op == QStringLiteral("*") || op == QStringLiteral("hypot") || op == QStringLiteral("min") || op == QStringLiteral("max")) {
                double result = op == QStringLiteral("hypot") ? std::abs(a) : a;
                for (qsizetype index = 1; index < operands.size(); ++index) {
                    const double b = evaluateExpression(operands.at(index), x);
                    if (!std::isfinite(b))
                        return b;
                    if (op == QStringLiteral("+"))
                        result += b;
                    else if (op == QStringLiteral("*"))
                        result *= b;
                    else if (op == QStringLiteral("hypot"))
                        result = std::hypot(result, b);
                    else if (op == QStringLiteral("min"))
                        result = std::min(result, b);
                    else
                        result = std::max(result, b);
                }
                return result;
            }
            if (operands.size() == 2) {
                const double b = evaluateExpression(operands.at(1), x);
                if (!std::isfinite(b))
                    return b;
                if (op == QStringLiteral("-"))
                    return a - b;
                if (op == QStringLiteral("/"))
                    return a / b;
                if (op == QStringLiteral("%"))
                    return std::fmod(a, b);
                if (op == QStringLiteral("pow"))
                    return std::pow(a, b);
                if (op == QStringLiteral(">"))
                    return a > b;
                if (op == QStringLiteral("<"))
                    return a < b;
                if (op == QStringLiteral(">="))
                    return a >= b;
                if (op == QStringLiteral("<="))
                    return a <= b;
                if (op == QStringLiteral("=="))
                    return a == b;
                if (op == QStringLiteral("!="))
                    return a != b;
                if (op == QStringLiteral("||"))
                    return a != 0 || b != 0;
                if (op == QStringLiteral("&&"))
                    return a != 0 && b != 0;
            }
            if (op == QStringLiteral("!"))
                return a == 0;
            if (op == QStringLiteral("sign"))
                return (a > 0) - (a < 0);
            if (op == QStringLiteral("ln"))
                return std::log(a);
            if (op == QStringLiteral("log"))
                return std::log10(a);
#define SYNTH_UNARY_FUNCTION(name) \
    if (op == QStringLiteral(#name)) \
        return std::name(a)
            SYNTH_UNARY_FUNCTION(abs);
            SYNTH_UNARY_FUNCTION(acos);
            SYNTH_UNARY_FUNCTION(acosh);
            SYNTH_UNARY_FUNCTION(asin);
            SYNTH_UNARY_FUNCTION(asinh);
            SYNTH_UNARY_FUNCTION(atan);
            SYNTH_UNARY_FUNCTION(atanh);
            SYNTH_UNARY_FUNCTION(ceil);
            SYNTH_UNARY_FUNCTION(cos);
            SYNTH_UNARY_FUNCTION(cosh);
            SYNTH_UNARY_FUNCTION(exp);
            SYNTH_UNARY_FUNCTION(floor);
            SYNTH_UNARY_FUNCTION(round);
            SYNTH_UNARY_FUNCTION(sin);
            SYNTH_UNARY_FUNCTION(sinh);
            SYNTH_UNARY_FUNCTION(sqrt);
            SYNTH_UNARY_FUNCTION(tan);
            SYNTH_UNARY_FUNCTION(tanh);
            SYNTH_UNARY_FUNCTION(trunc);
#undef SYNTH_UNARY_FUNCTION
            return std::numeric_limits<double>::quiet_NaN();
        }

    }

    struct ParameterRuntimeRegistry::Context {
        Api::V1::ParameterDefinition definition;
    };

    ParameterRuntimeRegistry &ParameterRuntimeRegistry::instance() {
        static ParameterRuntimeRegistry registry;
        return registry;
    }

    ParameterRuntimeRegistry::~ParameterRuntimeRegistry() = default;

    bool ParameterRuntimeRegistry::parameterInfo(const Api::V1::ParameterDefinition &definition, Core::ParameterInfo *result, QString *errorMessage) {
        if (!result || !validateExpression(definition.displayValueMappingExpression)
            || !validateExpression(definition.displayValueInverseMappingExpression)) {
            if (errorMessage)
                *errorMessage = QCoreApplication::translate("Synth::Internal::ParameterRuntimeRegistry", "Invalid parameter display expression");
            return false;
        }
        const auto json = QJsonDocument(definition.toJson().toObject()).toJson(QJsonDocument::Compact);
        const auto hash = XXH3_128bits(json.data(), json.size());
        const auto handle = QByteArray(reinterpret_cast<const char *>(&hash), sizeof(hash));
        {
            std::lock_guard lock(m_mutex);
            if (!m_contexts.contains(handle))
                m_contexts.insert(handle, std::make_shared<Context>(Context{definition}));
        }
        Core::ParameterInfo info;
        info.displayName = definition.name;
        info.defaultValue = definition.defaultValue;
        info.baselineValue = definition.baselineValue;
        info.showDefaultValue = definition.showBaseline;
        info.valueType = definition.fallbackOnDefaultValue ? Core::ParameterInfo::Relative : Core::ParameterInfo::Absolute;
        if (definition.fillMode == QStringLiteral("top"))
            info.fillMode = Core::ParameterInfo::TopFill;
        else if (definition.fillMode == QStringLiteral("bottom"))
            info.fillMode = Core::ParameterInfo::BottomFill;
        else if (definition.fillMode == QStringLiteral("baseline"))
            info.fillMode = Core::ParameterInfo::BaselineFill;
        info.userData = handle;
        info.toDisplayValue = &ParameterRuntimeRegistry::toDisplayValue;
        info.fromDisplayValue = &ParameterRuntimeRegistry::fromDisplayValue;
        info.toDisplayString = &ParameterRuntimeRegistry::toDisplayString;
        *result = std::move(info);
        return true;
    }

    void ParameterRuntimeRegistry::clear() {
        std::lock_guard lock(m_mutex);
        m_contexts.clear();
    }

    std::shared_ptr<ParameterRuntimeRegistry::Context> ParameterRuntimeRegistry::context(const QByteArray &handle) const {
        std::lock_guard lock(m_mutex);
        return m_contexts.value(handle);
    }

    double ParameterRuntimeRegistry::toDisplayValue(const Core::ParameterInfo &self, double value) {
        const auto runtime = instance().context(self.userData.toByteArray());
        const double result = runtime ? evaluateExpression(runtime->definition.displayValueMappingExpression, value) : value;
        return std::isfinite(result) ? result : value;
    }

    double ParameterRuntimeRegistry::fromDisplayValue(const Core::ParameterInfo &self, double value) {
        const auto runtime = instance().context(self.userData.toByteArray());
        const double result = runtime ? evaluateExpression(runtime->definition.displayValueInverseMappingExpression, value) : value;
        return std::clamp(std::isfinite(result) ? result : self.defaultValue, 0.0, 1.0);
    }

    QString ParameterRuntimeRegistry::toDisplayString(const Core::ParameterInfo &self, double value) {
        const auto runtime = instance().context(self.userData.toByteArray());
        if (!runtime)
            return QLocale().toString(value);
        const auto &definition = runtime->definition;
        return definition.displayValuePrefix + QLocale().toString(toDisplayValue(self, value), 'f', definition.displayValueDecimalPlaces) + definition.displayValueSuffix;
    }

}
