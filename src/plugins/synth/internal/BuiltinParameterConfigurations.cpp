// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BuiltinParameterConfigurations.h"

namespace Synth::Internal {

    namespace {

        ParameterConfiguration makeParameter(const QString &id, const QString &name,
                                             double defaultValue, bool showDefault,
                                             ParameterConfiguration::FillMode fillMode,
                                             ParameterConfiguration::ValueType valueType,
                                             double division, bool showDivision,
                                             const QString &displayExpression,
                                             const QString &displayInverseExpression,
                                             const QString &displayTemplate) {
            ParameterConfiguration result;
            result.setId(id);
            result.setArchitectureId(QStringLiteral("diffsinger"));
            result.setDisplayName(name);
            result.setDefaultValue(defaultValue);
            result.setShowDefaultValue(showDefault);
            result.setFillMode(fillMode);
            result.setValueType(valueType);
            result.setDivisionValue(division);
            result.setShowDivision(showDivision);
            result.setDisplayValueExpression(displayExpression);
            result.setDisplayValueInverseExpression(displayInverseExpression);
            result.setDisplayTextTemplate(displayTemplate);
            return result;
        }

        ParameterConfiguration makeDecibel(const QString &id, const QString &name) {
            return makeParameter(
                id, name, 1.0, false,
                ParameterConfiguration::BottomFill, ParameterConfiguration::Absolute,
                0.125, true, QStringLiteral("-96*(1-x)^0.8"),
                QStringLiteral("1-(-x/96)^1.25"),
                QStringLiteral("%.3f dB"));
        }

    }

    QList<ParameterConfiguration> BuiltinParameterConfigurations::all() {
        QList<ParameterConfiguration> result;
        result.reserve(9);
        result.append(makeParameter(
            QStringLiteral("expressiveness"), tr("Expressiveness"), 1.0, false,
            ParameterConfiguration::BottomFill, ParameterConfiguration::Relative,
            0.1, true, QStringLiteral("x"), QStringLiteral("x"),
            QStringLiteral("%.3f")));
        result.append(makeDecibel(QStringLiteral("energy"), tr("Energy")));
        result.append(makeDecibel(QStringLiteral("breathiness"), tr("Breathiness")));
        result.append(makeDecibel(QStringLiteral("voicing"), tr("Voicing")));
        result.append(makeParameter(
            QStringLiteral("tension"), tr("Tension"), 0.5, false,
            ParameterConfiguration::BottomFill, ParameterConfiguration::Absolute,
            0.1, true,
            QStringLiteral("sign(x-0.5)*10*(1-(1-abs(2*x-1))^0.7)"),
            QStringLiteral("0.5+sign(x)*(1-(1-abs(x)/10)^(1/0.7))/2"),
            QStringLiteral("%.3f")));
        result.append(makeParameter(
            QStringLiteral("mouth_opening"), tr("Mouth opening"), 0.0, false,
            ParameterConfiguration::BottomFill, ParameterConfiguration::Absolute,
            0.2, true, QStringLiteral("x"), QStringLiteral("x"),
            QStringLiteral("%.3f")));
        result.append(makeParameter(
            QStringLiteral("gender"), tr("Gender"), 0.5, true,
            ParameterConfiguration::BaselineFill, ParameterConfiguration::Relative,
            0.1, true, QStringLiteral("2*x-1"), QStringLiteral("(x+1)/2"),
            QStringLiteral("%.3f")));
        result.append(makeParameter(
            QStringLiteral("velocity"), tr("Velocity"), 0.5, true,
            ParameterConfiguration::BaselineFill, ParameterConfiguration::Relative,
            0.125, true, QStringLiteral("2^(2*x-1)"),
            QStringLiteral("(ln(x)/ln(2)+1)/2"), QStringLiteral("%.3f")));
        result.append(makeParameter(
            QStringLiteral("tone_shift"), tr("Tone shift"), 0.5, true,
            ParameterConfiguration::BaselineFill, ParameterConfiguration::Relative,
            1.0 / 12.0, true, QStringLiteral("2400*x-1200"),
            QStringLiteral("(x+1200)/2400"),
            QStringLiteral("%d cent")));
        return result;
    }

}
