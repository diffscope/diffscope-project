#include "BuiltinParameterConfigurations.h"

namespace Synth::Internal {

    namespace {

        ParameterConfiguration makeLinear(const QString &id, const QString &name, int minimum,
                                          int maximum, int defaultValue, bool showDefault,
                                          ParameterConfiguration::FillMode fillMode,
                                          ParameterConfiguration::ValueType valueType,
                                          int division, bool showDivision,
                                          const QString &displayExpression,
                                          const QString &displayInverseExpression,
                                          const QString &displayTemplate) {
            ParameterConfiguration result;
            result.setId(id);
            result.setArchitectureId(QStringLiteral("diffsinger"));
            result.setDisplayName(name);
            result.setMinimumValue(minimum);
            result.setMaximumValue(maximum);
            result.setDefaultValue(defaultValue);
            result.setShowDefaultValue(showDefault);
            result.setFillMode(fillMode);
            result.setValueType(valueType);
            result.setDivisionValue(division);
            result.setShowDivision(showDivision);
            result.setNormalizationExpression(
                QStringLiteral("(x-(%1))/(%2)").arg(minimum).arg(maximum - minimum));
            result.setDenormalizationExpression(
                QStringLiteral("x*(%1)+(%2)").arg(maximum - minimum).arg(minimum));
            result.setDisplayValueExpression(displayExpression);
            result.setDisplayValueInverseExpression(displayInverseExpression);
            result.setDisplayTextTemplate(displayTemplate);
            return result;
        }

        ParameterConfiguration makeDecibel(const QString &id, const QString &name) {
            auto result = makeLinear(
                id, name, -96000, 0, 0, false,
                ParameterConfiguration::BottomFill, ParameterConfiguration::Absolute,
                12000, true, QStringLiteral("x/1000"), QStringLiteral("x*1000"),
                QStringLiteral("%.3f dB"));
            // Match the established energy editor response: devote more visual space to values
            // near 0 dB while retaining a reversible mapping for the full supported range.
            result.setNormalizationExpression(QStringLiteral("1-((-x)/96000)^1.25"));
            result.setDenormalizationExpression(QStringLiteral("-96000*(1-x)^0.8"));
            return result;
        }

    }

    QList<ParameterConfiguration> BuiltinParameterConfigurations::all() {
        QList<ParameterConfiguration> result;
        result.reserve(9);
        result.append(makeLinear(
            QStringLiteral("expressiveness"), tr("Expressiveness"), 0, 1000, 1000, false,
            ParameterConfiguration::BottomFill, ParameterConfiguration::Relative,
            100, true, QStringLiteral("x/1000"), QStringLiteral("x*1000"),
            QStringLiteral("%.3f")));
        result.append(makeDecibel(QStringLiteral("energy"), tr("Energy")));
        result.append(makeDecibel(QStringLiteral("breathiness"), tr("Breathiness")));
        result.append(makeDecibel(QStringLiteral("voicing"), tr("Voicing")));
        result.append(makeLinear(
            QStringLiteral("tension"), tr("Tension"), -10000, 10000, 0, false,
            ParameterConfiguration::BottomFill, ParameterConfiguration::Absolute,
            2000, true, QStringLiteral("x/1000"), QStringLiteral("x*1000"),
            QStringLiteral("%.3f")));
        result.last().setNormalizationExpression(
            QStringLiteral("0.5+sign(x)*(1-(1-abs(x)/10000)^(1/0.7))/2"));
        result.last().setDenormalizationExpression(
            QStringLiteral("sign(x-0.5)*(1-(1-abs((x-0.5)*2))^0.7)*10000"));
        result.append(makeLinear(
            QStringLiteral("mouth_opening"), tr("Mouth opening"), 0, 1000, 0, false,
            ParameterConfiguration::BottomFill, ParameterConfiguration::Absolute,
            200, true, QStringLiteral("x/1000"), QStringLiteral("x*1000"),
            QStringLiteral("%.3f")));
        result.append(makeLinear(
            QStringLiteral("gender"), tr("Gender"), -1000, 1000, 0, true,
            ParameterConfiguration::BaselineFill, ParameterConfiguration::Relative,
            200, true, QStringLiteral("x/1000"), QStringLiteral("x*1000"),
            QStringLiteral("%.3f")));
        result.append(makeLinear(
            QStringLiteral("velocity"), tr("Velocity"), -1000, 1000, 0, true,
            ParameterConfiguration::BaselineFill, ParameterConfiguration::Relative,
            250, true, QStringLiteral("2^(x/1000)"),
            QStringLiteral("ln(x)/ln(2)*1000"), QStringLiteral("%.3f")));
        result.append(makeLinear(
            QStringLiteral("tone_shift"), tr("Tone shift"), -1200, 1200, 0, true,
            ParameterConfiguration::BaselineFill, ParameterConfiguration::Relative,
            200, true, QStringLiteral("x"), QStringLiteral("x"),
            QStringLiteral("%d cent")));
        return result;
    }

}
