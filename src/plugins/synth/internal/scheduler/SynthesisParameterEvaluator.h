#ifndef DIFFSCOPE_SYNTH_SYNTHESISPARAMETEREVALUATOR_H
#define DIFFSCOPE_SYNTH_SYNTHESISPARAMETEREVALUATOR_H

#include <memory>

#include <synth/ParameterConfiguration.h>

namespace dspx {
    class Parameter;
}

namespace Synth::Internal {

    class SynthesisParameterEvaluator {
    public:
        SynthesisParameterEvaluator(dspx::Parameter *parameter, int minimumTick, int maximumTick);
        ~SynthesisParameterEvaluator();

        double evaluate(double tick, double fallback) const;
        static double normalize(const ParameterConfiguration &configuration, double value);

    private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISPARAMETEREVALUATOR_H
