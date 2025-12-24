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

        void setPosUnchecked(int pos_);
        void setPos(int pos_);
        void setValueUnchecked(double value_);
        void setValue(double value_);

        static void setTempoSequence(Tempo *item, TempoSequence *tempoSequence);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TEMPO_P_H