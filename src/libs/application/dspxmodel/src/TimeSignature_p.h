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
        TimeSignature *previousItem{};
        TimeSignature *nextItem{};

        static void setTimeSignatureSequence(TimeSignature *item, TimeSignatureSequence *timeSignatureSequence);
        static void setPreviousItem(TimeSignature *item, TimeSignature *previousItem);
        static void setNextItem(TimeSignature *item, TimeSignature *nextItem);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TIMESIGNATURE_P_H