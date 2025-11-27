#ifndef DIFFSCOPE_DSPX_MODEL_TIMESIGNATURESEQUENCE_P_H
#define DIFFSCOPE_DSPX_MODEL_TIMESIGNATURESEQUENCE_P_H

#include <dspxmodel/TimeSignatureSequence.h>

#include <dspxmodel/private/PointSequenceData_p.h>
#include <dspxmodel/private/TimeSignature_p.h>

namespace dspx {

    class TimeSignatureSequencePrivate : public PointSequenceData<TimeSignatureSequence, TimeSignature, &TimeSignature::index, &TimeSignature::indexChanged> {
        Q_DECLARE_PUBLIC(TimeSignatureSequence)
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TIMESIGNATURESEQUENCE_P_H