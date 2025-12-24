#ifndef DIFFSCOPE_DSPX_MODEL_PHONEME_P_H
#define DIFFSCOPE_DSPX_MODEL_PHONEME_P_H

#include <dspxmodel/Phoneme.h>

namespace dspx {

    class PhonemeList;
    class ModelPrivate;

    class PhonemePrivate {
        Q_DECLARE_PUBLIC(Phoneme)
    public:
        Phoneme *q_ptr;
        ModelPrivate *pModel;
        QString language;
        int start{};
        QString token;
        bool onset{};
        PhonemeList *phonemeList{};

        static void setPhonemeList(Phoneme *item, PhonemeList *phonemeList);

        void setLanguageUnchecked(const QString &language);
        void setStartUnchecked(int start);
        void setTokenUnchecked(const QString &token);
        void setOnsetUnchecked(bool onset);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PHONEME_P_H