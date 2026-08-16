// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISPARAMETEREVALUATOR_H
#define DIFFSCOPE_SYNTH_SYNTHESISPARAMETEREVALUATOR_H

#include <memory>

namespace dspx {
    class Parameter;
}

namespace Synth::Internal {

    class SynthesisParameterEvaluator {
    public:
        SynthesisParameterEvaluator(dspx::Parameter *parameter, int minimumTick, int maximumTick);
        ~SynthesisParameterEvaluator();

        double evaluate(double tick, double fallback) const;

    private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISPARAMETEREVALUATOR_H
