#ifndef DIFFSCOPE_DSPX_MODEL_KEYSIGNATURE_P_H
#define DIFFSCOPE_DSPX_MODEL_KEYSIGNATURE_P_H

#include <dspxmodel/KeySignature.h>

namespace dspx {

    class KeySignaturePrivate {
        Q_DECLARE_PUBLIC(KeySignature)
    public:
        KeySignature *q_ptr;
        int pos;
        int mode;
        int tonality;
        KeySignature::AccidentalType accidentalType;
        KeySignatureSequence *keySignatureSequence{};
        KeySignature *previousItem{};
        KeySignature *nextItem{};

        static void setKeySignatureSequence(KeySignature *item, KeySignatureSequence *keySignatureSequence);
        static void setPreviousItem(KeySignature *item, KeySignature *previousItem);
        static void setNextItem(KeySignature *item, KeySignature *nextItem);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_KEYSIGNATURE_P_H
