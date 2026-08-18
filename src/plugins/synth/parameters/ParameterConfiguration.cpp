// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ParameterConfiguration.h"
#include "ParameterConfiguration_p.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include <QCoreApplication>

#include <synth/internal/private/ParameterExpressionUtils_p.h>

namespace Synth {

    namespace {

        QString translateError(const char *sourceText) {
            return QCoreApplication::translate("Synth::ParameterConfiguration", sourceText);
        }

        bool containsControlCharacter(const QString &value) {
            for (const auto character : value) {
                if (character.category() == QChar::Other_Control)
                    return true;
            }
            return false;
        }

        QString fillModeName(ParameterConfiguration::FillMode value) {
            switch (value) {
                case ParameterConfiguration::NoFill: return QStringLiteral("none");
                case ParameterConfiguration::TopFill: return QStringLiteral("top");
                case ParameterConfiguration::BottomFill: return QStringLiteral("bottom");
                case ParameterConfiguration::BaselineFill: return QStringLiteral("baseline");
            }
            return {};
        }

        bool parseFillMode(const QString &value, ParameterConfiguration::FillMode *result) {
            if (value == QStringLiteral("none")) *result = ParameterConfiguration::NoFill;
            else if (value == QStringLiteral("top")) *result = ParameterConfiguration::TopFill;
            else if (value == QStringLiteral("bottom")) *result = ParameterConfiguration::BottomFill;
            else if (value == QStringLiteral("baseline")) *result = ParameterConfiguration::BaselineFill;
            else return false;
            return true;
        }

        QString valueTypeName(ParameterConfiguration::ValueType value) {
            switch (value) {
                case ParameterConfiguration::Absolute: return QStringLiteral("absolute");
                case ParameterConfiguration::Relative: return QStringLiteral("relative");
            }
            return {};
        }

        bool parseValueType(const QString &value, ParameterConfiguration::ValueType *result) {
            if (value == QStringLiteral("absolute")) *result = ParameterConfiguration::Absolute;
            else if (value == QStringLiteral("relative")) *result = ParameterConfiguration::Relative;
            else return false;
            return true;
        }

        bool evaluateOrLinear(const QString &expression, double input, int minimum, int maximum,
                              bool inverse, double *result, int *errorPosition) {
            if (!expression.trimmed().isEmpty())
                return Internal::ParameterExpressionUtils::evaluate(expression, input, result, errorPosition);
            const auto range = static_cast<double>(maximum) - static_cast<double>(minimum);
            *result = inverse ? static_cast<double>(minimum) + input * range
                              : (input - static_cast<double>(minimum)) / range;
            return true;
        }

        bool evaluateOrIdentity(const QString &expression, double input, double *result,
                                int *errorPosition) {
            if (expression.trimmed().isEmpty()) {
                *result = input;
                return true;
            }
            return Internal::ParameterExpressionUtils::evaluate(expression, input, result, errorPosition);
        }

    }

    ParameterConfiguration::ParameterConfiguration() : d(new ParameterConfigurationData) {}
    ParameterConfiguration::ParameterConfiguration(const ParameterConfiguration &other) = default;
    ParameterConfiguration::ParameterConfiguration(ParameterConfiguration &&other) noexcept = default;
    ParameterConfiguration &ParameterConfiguration::operator=(const ParameterConfiguration &other) = default;
    ParameterConfiguration &ParameterConfiguration::operator=(ParameterConfiguration &&other) noexcept = default;
    ParameterConfiguration::~ParameterConfiguration() = default;

    QString ParameterConfiguration::id() const { return d->id; }
    void ParameterConfiguration::setId(const QString &id) { d->id = id; }
    QString ParameterConfiguration::architectureId() const { return d->architectureId; }
    void ParameterConfiguration::setArchitectureId(const QString &id) { d->architectureId = id; }
    QString ParameterConfiguration::displayName() const { return d->displayName; }
    void ParameterConfiguration::setDisplayName(const QString &name) { d->displayName = name; }
    int ParameterConfiguration::minimumValue() const { return d->minimumValue; }
    void ParameterConfiguration::setMinimumValue(int value) { d->minimumValue = value; }
    int ParameterConfiguration::maximumValue() const { return d->maximumValue; }
    void ParameterConfiguration::setMaximumValue(int value) { d->maximumValue = value; }
    bool ParameterConfiguration::showDefaultValue() const { return d->showDefaultValue; }
    void ParameterConfiguration::setShowDefaultValue(bool show) { d->showDefaultValue = show; }
    int ParameterConfiguration::defaultValue() const { return d->defaultValue; }
    void ParameterConfiguration::setDefaultValue(int value) { d->defaultValue = value; }
    ParameterConfiguration::FillMode ParameterConfiguration::fillMode() const { return d->fillMode; }
    void ParameterConfiguration::setFillMode(FillMode mode) { d->fillMode = mode; }
    ParameterConfiguration::ValueType ParameterConfiguration::valueType() const { return d->valueType; }
    void ParameterConfiguration::setValueType(ValueType type) { d->valueType = type; }
    bool ParameterConfiguration::showDivision() const { return d->showDivision; }
    void ParameterConfiguration::setShowDivision(bool show) { d->showDivision = show; }
    int ParameterConfiguration::divisionValue() const { return d->divisionValue; }
    void ParameterConfiguration::setDivisionValue(int value) { d->divisionValue = value; }
    QString ParameterConfiguration::normalizationExpression() const { return d->normalizationExpression; }
    void ParameterConfiguration::setNormalizationExpression(const QString &value) { d->normalizationExpression = value; }
    QString ParameterConfiguration::denormalizationExpression() const { return d->denormalizationExpression; }
    void ParameterConfiguration::setDenormalizationExpression(const QString &value) { d->denormalizationExpression = value; }
    QString ParameterConfiguration::displayValueExpression() const { return d->displayValueExpression; }
    void ParameterConfiguration::setDisplayValueExpression(const QString &value) { d->displayValueExpression = value; }
    QString ParameterConfiguration::displayValueInverseExpression() const { return d->displayValueInverseExpression; }
    void ParameterConfiguration::setDisplayValueInverseExpression(const QString &value) { d->displayValueInverseExpression = value; }
    QString ParameterConfiguration::displayTextTemplate() const { return d->displayTextTemplate; }
    void ParameterConfiguration::setDisplayTextTemplate(const QString &value) { d->displayTextTemplate = value; }

    bool ParameterConfiguration::validate(QStringList *errors) const {
        QStringList localErrors;
        if (d->id.trimmed().isEmpty() || containsControlCharacter(d->id))
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Parameter ID must not be empty or contain control characters")));
        if (d->id == QStringLiteral("pitch"))
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "The parameter ID 'pitch' is reserved and cannot be configured")));
        if (d->architectureId.trimmed().isEmpty() || containsControlCharacter(d->architectureId))
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Architecture ID must not be empty or contain control characters")));
        if (d->displayName.trimmed().isEmpty() || containsControlCharacter(d->displayName))
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Display name must not be empty or contain control characters")));
        if (d->minimumValue >= d->maximumValue)
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Minimum value must be less than maximum value")));
        if (d->showDefaultValue &&
            (d->defaultValue < d->minimumValue || d->defaultValue > d->maximumValue)) {
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Default value must be within the configured range")));
        }
        if (d->showDivision && d->divisionValue <= 0)
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Division value must be positive when divisions are shown")));
        if (d->fillMode < NoFill || d->fillMode > BaselineFill)
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Fill mode is invalid")));
        if (d->valueType < Absolute || d->valueType > Relative)
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Value type is invalid")));
        if (d->normalizationExpression.trimmed().isEmpty() !=
            d->denormalizationExpression.trimmed().isEmpty()) {
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Normalization and inverse normalization expressions must be configured together")));
        }
        if (d->displayValueExpression.trimmed().isEmpty() !=
            d->displayValueInverseExpression.trimmed().isEmpty()) {
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Display value mapping and inverse display value mapping expressions must be configured together")));
        }
        const auto expressions = {
            d->normalizationExpression,
            d->denormalizationExpression,
            d->displayValueExpression,
            d->displayValueInverseExpression,
        };
        if (std::any_of(expressions.begin(), expressions.end(), containsControlCharacter))
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Expressions must not contain control characters")));

        if (d->minimumValue < d->maximumValue) {
            double previous = -std::numeric_limits<double>::infinity();
            for (int index = 0; index <= 16; ++index) {
                const auto raw = static_cast<double>(d->minimumValue) +
                                 (static_cast<double>(d->maximumValue) - d->minimumValue) * index / 16.0;
                double normalized{};
                int errorPosition{};
                if (!evaluateOrLinear(d->normalizationExpression, raw, d->minimumValue,
                                      d->maximumValue, false, &normalized, &errorPosition)) {
                    localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Normalization expression is invalid at position %L1"))
                                           .arg(errorPosition));
                    break;
                }
                if (normalized < -1e-9 || normalized > 1.0 + 1e-9 || normalized + 1e-9 < previous) {
                    localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Normalization expression must be finite, increasing, and map into [0, 1]")));
                    break;
                }
                double roundTrip{};
                if (!evaluateOrLinear(d->denormalizationExpression, normalized, d->minimumValue,
                                      d->maximumValue, true, &roundTrip, &errorPosition) ||
                    std::abs(roundTrip - raw) > 1.0) {
                    localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Normalization expressions do not round-trip within one raw unit")));
                    break;
                }
                double display{};
                double displayRoundTrip{};
                if (!evaluateOrIdentity(d->displayValueExpression, raw, &display, &errorPosition) ||
                    !evaluateOrIdentity(d->displayValueInverseExpression, display,
                                        &displayRoundTrip, &errorPosition) ||
                    std::abs(displayRoundTrip - raw) > 1.0) {
                    localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Display value expressions are invalid or do not round-trip within one raw unit")));
                    break;
                }
                previous = normalized;
            }
        }

        int templateError{};
        if (!Internal::ParameterExpressionUtils::validateDisplayTemplate(d->displayTextTemplate,
                                                                        &templateError)) {
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Display text template contains an invalid placeholder at position %L1"))
                                   .arg(templateError));
        }

        if (errors)
            *errors = localErrors;
        return localErrors.isEmpty();
    }

    QJsonObject ParameterConfiguration::toJson() const {
        return {
            {QStringLiteral("id"), d->id},
            {QStringLiteral("architectureId"), d->architectureId},
            {QStringLiteral("displayName"), d->displayName},
            {QStringLiteral("minimumValue"), d->minimumValue},
            {QStringLiteral("maximumValue"), d->maximumValue},
            {QStringLiteral("showDefaultValue"), d->showDefaultValue},
            {QStringLiteral("defaultValue"), d->defaultValue},
            {QStringLiteral("fillMode"), fillModeName(d->fillMode)},
            {QStringLiteral("valueType"), valueTypeName(d->valueType)},
            {QStringLiteral("showDivision"), d->showDivision},
            {QStringLiteral("divisionValue"), d->divisionValue},
            {QStringLiteral("normalizationExpression"), d->normalizationExpression},
            {QStringLiteral("denormalizationExpression"), d->denormalizationExpression},
            {QStringLiteral("displayValueExpression"), d->displayValueExpression},
            {QStringLiteral("displayValueInverseExpression"), d->displayValueInverseExpression},
            {QStringLiteral("displayTextTemplate"), d->displayTextTemplate},
        };
    }

    bool ParameterConfiguration::fromJson(const QJsonObject &object, ParameterConfiguration *result,
                                          QString *errorMessage) {
        if (!result) {
            if (errorMessage) *errorMessage = translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Result pointer must not be null"));
            return false;
        }
        ParameterConfiguration value;
        const auto readString = [&](const QString &key, QString *target) {
            const auto item = object.value(key);
            if (!item.isString()) {
                if (errorMessage) *errorMessage = translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Field '%1' must be a string")).arg(key);
                return false;
            }
            *target = item.toString();
            return true;
        };
        const auto readInt = [&](const QString &key, int *target) {
            const auto item = object.value(key);
            if (!item.isDouble()) {
                if (errorMessage) *errorMessage = translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Field '%1' must be an integer")).arg(key);
                return false;
            }
            *target = item.toInt();
            if (item.toDouble() != static_cast<double>(*target)) {
                if (errorMessage) *errorMessage = translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Field '%1' must be an integer")).arg(key);
                return false;
            }
            return true;
        };
        const auto readBool = [&](const QString &key, bool *target) {
            const auto item = object.value(key);
            if (!item.isBool()) {
                if (errorMessage) *errorMessage = translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Field '%1' must be a boolean")).arg(key);
                return false;
            }
            *target = item.toBool();
            return true;
        };
        QString fillMode, valueType;
        if (!readString(QStringLiteral("id"), &value.d->id) ||
            !readString(QStringLiteral("architectureId"), &value.d->architectureId) ||
            !readString(QStringLiteral("displayName"), &value.d->displayName) ||
            !readInt(QStringLiteral("minimumValue"), &value.d->minimumValue) ||
            !readInt(QStringLiteral("maximumValue"), &value.d->maximumValue) ||
            !readBool(QStringLiteral("showDefaultValue"), &value.d->showDefaultValue) ||
            !readInt(QStringLiteral("defaultValue"), &value.d->defaultValue) ||
            !readString(QStringLiteral("fillMode"), &fillMode) ||
            !readString(QStringLiteral("valueType"), &valueType) ||
            !readBool(QStringLiteral("showDivision"), &value.d->showDivision) ||
            !readInt(QStringLiteral("divisionValue"), &value.d->divisionValue) ||
            !readString(QStringLiteral("normalizationExpression"), &value.d->normalizationExpression) ||
            !readString(QStringLiteral("denormalizationExpression"), &value.d->denormalizationExpression) ||
            !readString(QStringLiteral("displayValueExpression"), &value.d->displayValueExpression) ||
            !readString(QStringLiteral("displayValueInverseExpression"), &value.d->displayValueInverseExpression) ||
            !readString(QStringLiteral("displayTextTemplate"), &value.d->displayTextTemplate) ||
            !parseFillMode(fillMode, &value.d->fillMode) || !parseValueType(valueType, &value.d->valueType)) {
            if (errorMessage && errorMessage->isEmpty())
                *errorMessage = translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Parameter configuration contains an invalid enum value"));
            return false;
        }
        QStringList errors;
        if (!value.validate(&errors)) {
            if (errorMessage) *errorMessage = errors.join(u'\n');
            return false;
        }
        *result = std::move(value);
        return true;
    }

    bool ParameterConfiguration::operator==(const ParameterConfiguration &other) const {
        return d.constData() == other.d.constData() || toJson() == other.toJson();
    }
    bool ParameterConfiguration::operator!=(const ParameterConfiguration &other) const {
        return !(*this == other);
    }

}

#include "moc_ParameterConfiguration.cpp"
