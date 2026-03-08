#ifndef DIFFSCOPE_DSPX_MODEL_LABELSEQUENCE_P_H
#define DIFFSCOPE_DSPX_MODEL_LABELSEQUENCE_P_H

#include <dspxmodel/LabelSequence.h>

#include <dspxmodel/private/PointSequenceData_p.h>
#include <dspxmodel/private/Label_p.h>

namespace dspx {

    class LabelSequencePrivate : public PointSequenceData<LabelSequence, Label, &Label::pos, &Label::posChanged, &LabelPrivate::setLabelSequence, &LabelPrivate::setPreviousItem, &LabelPrivate::setNextItem> {
        Q_DECLARE_PUBLIC(LabelSequence)
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_LABELSEQUENCE_P_H