#ifndef DIFFSCOPE_DSPX_MODEL_PHONEMELIST_P_H
#define DIFFSCOPE_DSPX_MODEL_PHONEMELIST_P_H

#include <dspxmodel/PhonemeList.h>
#include <dspxmodel/private/ListData_p.h>
#include <dspxmodel/private/Phoneme_p.h>

namespace dspx {

    class PhonemeInfo;

    class PhonemeListPrivate : public ListData<PhonemeList, Phoneme, &PhonemePrivate::setPhonemeList> {
        Q_DECLARE_PUBLIC(PhonemeList)
    public:
        PhonemeInfo *phonemeInfo{};
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PHONEMELIST_P_H