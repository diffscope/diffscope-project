#ifndef DIFFSCOPE_DSPX_MODEL_KEYSIGNATURESEQUENCE_P_H
#define DIFFSCOPE_DSPX_MODEL_KEYSIGNATURESEQUENCE_P_H

#include <dspxmodel/KeySignatureSequence.h>

#include <dspxmodel/private/PointSequenceData_p.h>
#include <dspxmodel/private/KeySignature_p.h>

namespace dspx {

    class KeySignatureSequencePrivate : public PointSequenceData<KeySignatureSequence, KeySignature, &KeySignature::pos, &KeySignature::posChanged, &KeySignaturePrivate::setKeySignatureSequence, &KeySignaturePrivate::setPreviousItem, &KeySignaturePrivate::setNextItem> {
        Q_DECLARE_PUBLIC(KeySignatureSequence)
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_KEYSIGNATURESEQUENCE_P_H
