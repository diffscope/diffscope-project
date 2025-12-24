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

        void setIndexUnchecked(int index_);
        void setIndex(int index_);
        void setNumeratorUnchecked(int numerator_);
        void setNumerator(int numerator_);
        void setDenominatorUnchecked(int denominator_);
        void setDenominator(int denominator_);

        static void setTimeSignatureSequence(TimeSignature *item, TimeSignatureSequence *timeSignatureSequence);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TIMESIGNATURE_P_H