// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ParameterConfiguration.h"
#include "ParameterConfiguration_p.h"

#include <algorithm>
#include <cmath>
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
    bool ParameterConfiguration::showDefaultValue() const { return d->showDefaultValue; }
    void ParameterConfiguration::setShowDefaultValue(bool show) { d->showDefaultValue = show; }
    double ParameterConfiguration::defaultValue() const { return d->defaultValue; }
    void ParameterConfiguration::setDefaultValue(double value) { d->defaultValue = value; }
    ParameterConfiguration::FillMode ParameterConfiguration::fillMode() const { return d->fillMode; }
    void ParameterConfiguration::setFillMode(FillMode mode) { d->fillMode = mode; }
    ParameterConfiguration::ValueType ParameterConfiguration::valueType() const { return d->valueType; }
    void ParameterConfiguration::setValueType(ValueType type) { d->valueType = type; }
    bool ParameterConfiguration::showDivision() const { return d->showDivision; }
    void ParameterConfiguration::setShowDivision(bool show) { d->showDivision = show; }
    double ParameterConfiguration::divisionValue() const { return d->divisionValue; }
    void ParameterConfiguration::setDivisionValue(double value) { d->divisionValue = value; }
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
        if (!std::isfinite(d->defaultValue) || d->defaultValue < 0.0 || d->defaultValue > 1.0)
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Default value must be finite and within [0, 1]")));
        if (d->showDivision && (!std::isfinite(d->divisionValue) || d->divisionValue <= 0.0 || d->divisionValue > 1.0))
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Division value must be finite and within (0, 1] when divisions are shown")));
        if (d->fillMode < NoFill || d->fillMode > BaselineFill)
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Fill mode is invalid")));
        if (d->valueType < Absolute || d->valueType > Relative)
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Value type is invalid")));
        if (d->displayValueExpression.trimmed().isEmpty() !=
            d->displayValueInverseExpression.trimmed().isEmpty()) {
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Display value mapping and inverse display value mapping expressions must be configured together")));
        }
        const auto expressions = {
            d->displayValueExpression,
            d->displayValueInverseExpression,
        };
        if (std::any_of(expressions.begin(), expressions.end(), containsControlCharacter))
            localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Expressions must not contain control characters")));

        {
            for (int index = 0; index <= 16; ++index) {
                const double value = static_cast<double>(index) / 16.0;
                int errorPosition{};
                double display{};
                double displayRoundTrip{};
                if (!evaluateOrIdentity(d->displayValueExpression, value, &display, &errorPosition) ||
                    !std::isfinite(display) ||
                    !evaluateOrIdentity(d->displayValueInverseExpression, display,
                                        &displayRoundTrip, &errorPosition) ||
                    !std::isfinite(displayRoundTrip) ||
                    std::abs(displayRoundTrip - value) > 1e-8) {
                    localErrors.append(translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Display value expressions must be finite, invertible, and round-trip normalized values")));
                    break;
                }
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
            {QStringLiteral("showDefaultValue"), d->showDefaultValue},
            {QStringLiteral("defaultValue"), d->defaultValue},
            {QStringLiteral("fillMode"), fillModeName(d->fillMode)},
            {QStringLiteral("valueType"), valueTypeName(d->valueType)},
            {QStringLiteral("showDivision"), d->showDivision},
            {QStringLiteral("divisionValue"), d->divisionValue},
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
        const auto readDouble = [&](const QString &key, double *target) {
            const auto item = object.value(key);
            if (!item.isDouble() || !std::isfinite(item.toDouble())) {
                if (errorMessage) *errorMessage = translateError(QT_TRANSLATE_NOOP("Synth::ParameterConfiguration", "Field '%1' must be a finite number")).arg(key);
                return false;
            }
            *target = item.toDouble();
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
            !readBool(QStringLiteral("showDefaultValue"), &value.d->showDefaultValue) ||
            !readDouble(QStringLiteral("defaultValue"), &value.d->defaultValue) ||
            !readString(QStringLiteral("fillMode"), &fillMode) ||
            !readString(QStringLiteral("valueType"), &valueType) ||
            !readBool(QStringLiteral("showDivision"), &value.d->showDivision) ||
            !readDouble(QStringLiteral("divisionValue"), &value.d->divisionValue) ||
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
