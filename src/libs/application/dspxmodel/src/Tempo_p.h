#ifndef DIFFSCOPE_DSPX_MODEL_TEMPO_P_H
#define DIFFSCOPE_DSPX_MODEL_TEMPO_P_H

#include <dspxmodel/Tempo.h>

namespace dspx {

    class TempoPrivate {
        Q_DECLARE_PUBLIC(Tempo)
    public:
        Tempo *q_ptr;
        int pos;
        double value;
        TempoSequence *tempoSequence{};

        static void setTempoSequence(Tempo *item, TempoSequence *tempoSequence);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TEMPO_P_H