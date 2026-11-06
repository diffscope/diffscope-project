// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSCENARIO_P_H
#define DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSCENARIO_P_H

#include <dspxproject/ParameterResamplingScenario.h>

#include <dspxproject/ParameterResamplingStrategy.h>

namespace DspxProject {

    class ParameterResamplingScenarioPrivate {
        Q_DECLARE_PUBLIC(ParameterResamplingScenario)
    public:
        ParameterResamplingScenario *q_ptr;
        ParameterResamplingStrategy *const strategy;
        const ParameterResamplingScenario::Preferences preferences;

        bool confirmResampling(ParameterResamplingStrategy::Operation operation, QQuickWindow *window, bool requiresResampling) const;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSCENARIO_P_H
