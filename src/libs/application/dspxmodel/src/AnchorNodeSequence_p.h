#ifndef DIFFSCOPE_DSPX_MODEL_ANCHORNODESEQUENCE_P_H
#define DIFFSCOPE_DSPX_MODEL_ANCHORNODESEQUENCE_P_H

#include <dspxmodel/AnchorNodeSequence.h>

#include <dspxmodel/private/PointSequenceData_p.h>
#include <dspxmodel/private/AnchorNode_p.h>

namespace dspx {

    class AnchorNodeSequencePrivate : public PointSequenceData<AnchorNodeSequence, AnchorNode, &AnchorNode::x, &AnchorNode::xChanged> {
        Q_DECLARE_PUBLIC(AnchorNodeSequence)
    public:
        ParamCurveAnchor *paramCurveAnchor{};
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_ANCHORNODESEQUENCE_P_H