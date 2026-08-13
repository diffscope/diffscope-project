#include "ParameterRuntimeRegistry.h"

#include <tinyexpr.h>

#include <algorithm>
#include <cmath>
#include <mutex>

#include <QCoreApplication>
#include <QCryptographicHash>
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
                    *errorMessage = translateError(QT_TRANSLATE_NOOP("Synth::Internal::ParameterRuntimeRegistry", "The parameter expression is invalid at position %L1."))
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

        int boundedRaw(const Core::ParameterInfo &info, double value) {
            if (!std::isfinite(value))
                return std::clamp(info.defaultValue, info.bottomValue, info.topValue);
            const auto rounded = std::round(value);
            if (rounded <= static_cast<double>(info.bottomValue))
                return info.bottomValue;
            if (rounded >= static_cast<double>(info.topValue))
                return info.topValue;
            return static_cast<int>(rounded);
        }

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
        explicit Context(const ParameterConfiguration &source, const QByteArray &json)
            : configuration(source), normalizedJson(json) {
        }

        bool compile(QString *errorMessage) {
            return normalization.compile(configuration.normalizationExpression(), errorMessage) &&
                   denormalization.compile(configuration.denormalizationExpression(), errorMessage) &&
                   display.compile(configuration.displayValueExpression(), errorMessage) &&
                   inverseDisplay.compile(configuration.displayValueInverseExpression(), errorMessage);
        }

        double linearNormalize(double raw) const {
            const auto minimum = static_cast<double>(configuration.minimumValue());
            const auto range = static_cast<double>(configuration.maximumValue()) - minimum;
            return range > 0.0 ? (raw - minimum) / range : 0.0;
        }

        double linearDenormalize(double normalized) const {
            return static_cast<double>(configuration.minimumValue()) + normalized *
                       (static_cast<double>(configuration.maximumValue()) - configuration.minimumValue());
        }

        ParameterConfiguration configuration;
        QByteArray normalizedJson;
        CompiledExpression normalization;
        CompiledExpression denormalization;
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
            if (errorMessage) *errorMessage = translateError(QT_TRANSLATE_NOOP("Synth::Internal::ParameterRuntimeRegistry", "Result pointer must not be null."));
            return false;
        }
        QStringList validationErrors;
        if (!configuration.validate(&validationErrors)) {
            if (errorMessage) *errorMessage = validationErrors.join(u'\n');
            return false;
        }

        const auto normalizedJson = QJsonDocument(configuration.toJson()).toJson(QJsonDocument::Compact);
        auto handle = QCryptographicHash::hash(normalizedJson, QCryptographicHash::Sha512);
        std::shared_ptr<Context> runtime;
        {
            std::lock_guard lock(m_mutex);
            quint32 collisionIndex{};
            while (true) {
                const auto existing = m_contexts.value(handle);
                if (!existing) {
                    runtime = std::make_shared<Context>(configuration, normalizedJson);
                    if (!runtime->compile(errorMessage))
                        return false;
                    m_contexts.insert(handle, runtime);
                    break;
                }
                if (existing->normalizedJson == normalizedJson) {
                    runtime = existing;
                    break;
                }
                ++collisionIndex;
                handle = QCryptographicHash::hash(
                    normalizedJson + QByteArray::number(collisionIndex), QCryptographicHash::Sha512
                );
            }
        }

        Core::ParameterInfo info;
        info.displayName = configuration.displayName();
        info.bottomValue = configuration.minimumValue();
        info.topValue = configuration.maximumValue();
        info.defaultValue = std::clamp(configuration.defaultValue(),
                                       configuration.minimumValue(),
                                       configuration.maximumValue());
        info.fillMode = coreFillMode(configuration.fillMode());
        info.valueType = coreValueType(configuration.valueType());
        info.divisionValue = configuration.divisionValue();
        info.showDefaultValue = configuration.showDefaultValue();
        info.showDivision = configuration.showDivision();
        info.userData = handle;
        info.normalize = &ParameterRuntimeRegistry::normalize;
        info.denormalize = &ParameterRuntimeRegistry::denormalize;
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

    double ParameterRuntimeRegistry::normalize(const Core::ParameterInfo &self, int value) {
        auto runtime = instance().context(self.userData.toByteArray());
        if (!runtime) {
            const auto range = static_cast<double>(self.topValue) - self.bottomValue;
            return range > 0.0 ? std::clamp((value - self.bottomValue) / range, 0.0, 1.0) : 0.0;
        }
        double result{};
        if (!runtime->normalization.evaluate(value, &result) || result < 0.0 || result > 1.0)
            result = runtime->linearNormalize(value);
        return std::clamp(result, 0.0, 1.0);
    }

    int ParameterRuntimeRegistry::denormalize(const Core::ParameterInfo &self, double value) {
        auto runtime = instance().context(self.userData.toByteArray());
        const auto normalized = std::clamp(value, 0.0, 1.0);
        if (!runtime) {
            return boundedRaw(self, self.bottomValue + normalized *
                                        (static_cast<double>(self.topValue) - self.bottomValue));
        }
        double result{};
        if (!runtime->denormalization.evaluate(normalized, &result)
            || result < self.bottomValue || result > self.topValue)
            result = runtime->linearDenormalize(normalized);
        return boundedRaw(self, result);
    }

    double ParameterRuntimeRegistry::toDisplayValue(const Core::ParameterInfo &self, int value) {
        auto runtime = instance().context(self.userData.toByteArray());
        if (!runtime)
            return value;
        double result{};
        if (!runtime->display.evaluate(value, &result))
            result = value;
        return std::isfinite(result) ? result : static_cast<double>(value);
    }

    int ParameterRuntimeRegistry::fromDisplayValue(const Core::ParameterInfo &self, double value) {
        auto runtime = instance().context(self.userData.toByteArray());
        if (!runtime)
            return boundedRaw(self, value);
        double result{};
        if (!runtime->inverseDisplay.evaluate(value, &result)
            || result < self.bottomValue || result > self.topValue)
            result = value;
        return boundedRaw(self, result);
    }

    QString ParameterRuntimeRegistry::toDisplayString(const Core::ParameterInfo &self, int value) {
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
