#ifndef DIFFSCOPE_DSPX_MODEL_LABEL_P_H
#define DIFFSCOPE_DSPX_MODEL_LABEL_P_H

#include <dspxmodel/Label.h>

namespace dspx {

    class LabelPrivate {
        Q_DECLARE_PUBLIC(Label)
    public:
        Label *q_ptr;
        int pos;
        QString text;
        LabelSequence *labelSequence{};



        static void setLabelSequence(Label *item, LabelSequence *labelSequence);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_LABEL_P_H