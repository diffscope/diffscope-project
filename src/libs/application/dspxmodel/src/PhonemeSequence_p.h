#ifndef DIFFSCOPE_DSPX_MODEL_PHONEMESEQUENCE_P_H
#define DIFFSCOPE_DSPX_MODEL_PHONEMESEQUENCE_P_H

#include <dspxmodel/PhonemeSequence.h>

#include <dspxmodel/private/PointSequenceData_p.h>
#include <dspxmodel/private/Phoneme_p.h>

namespace dspx {

    class PhonemeInfo;

    class PhonemeSequencePrivate : public PointSequenceData<PhonemeSequence, Phoneme, &Phoneme::start, &Phoneme::startChanged, &PhonemePrivate::setPhonemeSequence, &PhonemePrivate::setPreviousItem, &PhonemePrivate::setNextItem> {
        Q_DECLARE_PUBLIC(PhonemeSequence)
    public:
        PhonemeInfo *phonemeInfo{};
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PHONEMESEQUENCE_P_H
