#ifndef DIFFSCOPE_DSPX_MODEL_TIMESIGNATURE_P_H
#define DIFFSCOPE_DSPX_MODEL_TIMESIGNATURE_P_H

#include <dspxmodel/TimeSignature.h>

namespace dspx {

    class TimeSignaturePrivate {
        Q_DECLARE_PUBLIC(TimeSignature)
    public:
        TimeSignature *q_ptr;
        int index;
        int numerator;
        int denominator;
        TimeSignatureSequence *timeSignatureSequence{};

        static void setTimeSignatureSequence(TimeSignature *item, TimeSignatureSequence *timeSignatureSequence);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TIMESIGNATURE_P_H