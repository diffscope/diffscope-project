#ifndef DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_P_H
#define DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_P_H

#include <dspxmodel/PhonemeInfo.h>

namespace dspx {

    class Note;
    class ModelPrivate;

    class PhonemeInfoPrivate {
        Q_DECLARE_PUBLIC(PhonemeInfo)
    public:
        PhonemeInfo *q_ptr;
        ModelPrivate *pModel;
        PhonemeList *edited{};
        PhonemeList *original{};
        Note *note{};

        static void setNote(PhonemeInfo *item, Note *note);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_P_H