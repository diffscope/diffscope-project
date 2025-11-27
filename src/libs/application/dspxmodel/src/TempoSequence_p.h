#ifndef DIFFSCOPE_DSPX_MODEL_TEMPOSEQUENCE_P_H
#define DIFFSCOPE_DSPX_MODEL_TEMPOSEQUENCE_P_H

#include <dspxmodel/TempoSequence.h>

#include <dspxmodel/private/PointSequenceData_p.h>
#include <dspxmodel/private/Tempo_p.h>

namespace dspx {

    class TempoSequencePrivate : public PointSequenceData<TempoSequence, Tempo, &Tempo::pos, &Tempo::posChanged> {
        Q_DECLARE_PUBLIC(TempoSequence)
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TEMPOSEQUENCE_P_H