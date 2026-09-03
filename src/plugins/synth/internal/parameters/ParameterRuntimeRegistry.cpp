// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ParameterRuntimeRegistry.h"

#include <tinyexpr.h>

#include <xxhash.h>

#include <algorithm>
#include <cmath>
#include <mutex>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QLocale>

#include <synth/internal/private/ParameterExpressionUtils_p.h>

namespace Synth::Internal {

    namespace {

        QString translateError(const char *sourceText) {
            return QCoreApplication::translate("Synth::Internal::ParameterRuntimeRegistry", sourceText);
        }

        struct CompiledExpression {
            double variable{};
            te_expr *expression{};
            std::mutex mutex;

            CompiledExpression() = default;
            CompiledExpression(const CompiledExpression &) = delete;
            CompiledExpression &operator=(const CompiledExpression &) = delete;
            ~CompiledExpression() { te_free(expression); }

            bool compile(const QString &source, QString *errorMessage) {
                if (source.trimmed().isEmpty())
                    return true;
                int position{};
                expression = ParameterExpressionUtils::compile(source, &variable, &position);
                if (expression)
                    return true;
                if (errorMessage)
                    *errorMessage = translateError(QT_TRANSLATE_NOOP("Synth::Internal::ParameterRuntimeRegistry", "The parameter expression is invalid at position %L1"))
                                        .arg(position);
                return false;
            }

            bool evaluate(double input, double *output) {
                if (!expression)
                    return false;
                std::lock_guard lock(mutex);
                variable = input;
                const auto value = te_eval(expression);
                if (!std::isfinite(value))
                    return false;
                *output = value;
                return true;
            }
        };

        Core::ParameterInfo::FillMode coreFillMode(ParameterConfiguration::FillMode mode) {
            switch (mode) {
                case ParameterConfiguration::TopFill: return Core::ParameterInfo::TopFill;
                case ParameterConfiguration::BottomFill: return Core::ParameterInfo::BottomFill;
                case ParameterConfiguration::BaselineFill: return Core::ParameterInfo::BaselineFill;
                case ParameterConfiguration::NoFill: return Core::ParameterInfo::NoFill;
            }
            return Core::ParameterInfo::NoFill;
        }

        Core::ParameterInfo::ValueType coreValueType(ParameterConfiguration::ValueType type) {
            return type == ParameterConfiguration::Relative ? Core::ParameterInfo::Relative
                                                            : Core::ParameterInfo::Absolute;
        }

    }

    struct ParameterRuntimeRegistry::Context {
        explicit Context(const ParameterConfiguration &source)
            : configuration(source) {
        }

        bool compile(QString *errorMessage) {
            return display.compile(configuration.displayValueExpression(), errorMessage) &&
                   inverseDisplay.compile(configuration.displayValueInverseExpression(), errorMessage);
        }

        ParameterConfiguration configuration;
        CompiledExpression display;
        CompiledExpression inverseDisplay;
    };

    ParameterRuntimeRegistry &ParameterRuntimeRegistry::instance() {
        static ParameterRuntimeRegistry registry;
        return registry;
    }

    ParameterRuntimeRegistry::~ParameterRuntimeRegistry() = default;

    bool ParameterRuntimeRegistry::parameterInfo(const ParameterConfiguration &configuration,
                                                 Core::ParameterInfo *result,
                                                 QString *errorMessage) {
        if (!result) {
            if (errorMessage) *errorMessage = translateError(QT_TRANSLATE_NOOP("Synth::Internal::ParameterRuntimeRegistry", "Result pointer must not be null"));
            return false;
        }
        QStringList validationErrors;
        if (!configuration.validate(&validationErrors)) {
            if (errorMessage) *errorMessage = validationErrors.join(u'\n');
            return false;
        }

        const auto normalizedJson = QJsonDocument(configuration.toJson()).toJson(QJsonDocument::Compact);
        const auto hashResult = XXH3_128bits(normalizedJson.data(), normalizedJson.size());
        const auto handle = QByteArray(reinterpret_cast<const char *>(&hashResult), sizeof(hashResult));
        std::shared_ptr<Context> runtime;
        {
            std::lock_guard lock(m_mutex);
            runtime = m_contexts.value(handle);
            if (!runtime) {
                runtime = std::make_shared<Context>(configuration);
                if (!runtime->compile(errorMessage))
                    return false;
                m_contexts.insert(handle, runtime);
            }
        }

        Core::ParameterInfo info;
        info.displayName = configuration.displayName();
        info.defaultValue = configuration.defaultValue();
        info.fillMode = coreFillMode(configuration.fillMode());
        info.valueType = coreValueType(configuration.valueType());
        info.divisionValue = configuration.divisionValue();
        info.showDefaultValue = configuration.showDefaultValue();
        info.showDivision = configuration.showDivision();
        info.userData = handle;
        info.toDisplayValue = &ParameterRuntimeRegistry::toDisplayValue;
        info.fromDisplayValue = &ParameterRuntimeRegistry::fromDisplayValue;
        info.toDisplayString = &ParameterRuntimeRegistry::toDisplayString;
        *result = std::move(info);
        return true;
    }

    void ParameterRuntimeRegistry::clear() {
        QHash<QByteArray, std::shared_ptr<Context>> oldContexts;
        {
            std::lock_guard lock(m_mutex);
            oldContexts.swap(m_contexts);
        }
        // Destructing outside the registry mutex prevents an expression destructor from extending
        // the critical section during shutdown.
    }

    std::shared_ptr<ParameterRuntimeRegistry::Context>
        ParameterRuntimeRegistry::context(const QByteArray &handle) const {
        std::lock_guard lock(m_mutex);
        return m_contexts.value(handle);
    }

    double ParameterRuntimeRegistry::toDisplayValue(const Core::ParameterInfo &self, double value) {
        auto runtime = instance().context(self.userData.toByteArray());
        if (!runtime)
            return value;
        double result{};
        if (!runtime->display.evaluate(value, &result))
            result = value;
        return std::isfinite(result) ? result : value;
    }

    double ParameterRuntimeRegistry::fromDisplayValue(const Core::ParameterInfo &self, double value) {
        auto runtime = instance().context(self.userData.toByteArray());
        if (!runtime)
            return std::clamp(value, 0.0, 1.0);
        double result{};
        if (!runtime->inverseDisplay.evaluate(value, &result) || result < 0.0 || result > 1.0)
            result = value;
        if (!std::isfinite(result))
            result = self.defaultValue;
        return std::clamp(result, 0.0, 1.0);
    }

    QString ParameterRuntimeRegistry::toDisplayString(const Core::ParameterInfo &self, double value) {
        auto runtime = instance().context(self.userData.toByteArray());
        if (!runtime)
            return QLocale().toString(value);
        double displayValue{};
        if (!runtime->display.evaluate(value, &displayValue))
            displayValue = value;
        if (!std::isfinite(displayValue))
            displayValue = value;
        return ParameterExpressionUtils::formatDisplayValue(
            runtime->configuration.displayTextTemplate(), displayValue
        );
    }

}
