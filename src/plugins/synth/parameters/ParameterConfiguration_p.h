// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_PARAMETERCONFIGURATION_P_H
#define DIFFSCOPE_SYNTH_PARAMETERCONFIGURATION_P_H

#include <synth/ParameterConfiguration.h>

#include <QSharedData>

namespace Synth {

    class ParameterConfigurationData : public QSharedData {
    public:
        QString id;
        QString architectureId;
        QString displayName;
        bool showDefaultValue{false};
        double defaultValue{};
        ParameterConfiguration::FillMode fillMode{ParameterConfiguration::NoFill};
        ParameterConfiguration::ValueType valueType{ParameterConfiguration::Absolute};
        bool showDivision{true};
        double divisionValue{0.2};
        QString displayValueExpression;
        QString displayValueInverseExpression;
        QString displayTextTemplate{QStringLiteral("%d")};
    };

}

#endif // DIFFSCOPE_SYNTH_PARAMETERCONFIGURATION_P_H
