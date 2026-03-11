#ifndef DIFFSCOPE_DSPX_MODEL_PHONEME_P_H
#define DIFFSCOPE_DSPX_MODEL_PHONEME_P_H

#include <dspxmodel/Phoneme.h>

namespace dspx {

    class PhonemeSequence;
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
        PhonemeSequence *phonemeSequence{};
        Phoneme *previousItem{};
        Phoneme *nextItem{};

        static void setPhonemeSequence(Phoneme *item, PhonemeSequence *phonemeSequence);
        static void setPreviousItem(Phoneme *item, Phoneme *previousItem);
        static void setNextItem(Phoneme *item, Phoneme *nextItem);

        void setLanguageUnchecked(const QString &language);
        void setStartUnchecked(int start);
        void setTokenUnchecked(const QString &token);
        void setOnsetUnchecked(bool onset);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PHONEME_P_H