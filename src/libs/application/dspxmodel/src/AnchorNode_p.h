#ifndef DIFFSCOPE_DSPX_MODEL_ANCHORNODE_P_H
#define DIFFSCOPE_DSPX_MODEL_ANCHORNODE_P_H

#include <dspxmodel/AnchorNode.h>

namespace dspx {

    class AnchorNodePrivate {
        Q_DECLARE_PUBLIC(AnchorNode)
    public:
        AnchorNode *q_ptr;
        AnchorNode::InterpolationMode interp;
        int x;
        int y;
        AnchorNodeSequence *anchorNodeSequence{};

        void setInterpUnchecked(AnchorNode::InterpolationMode interp_);
        void setInterp(AnchorNode::InterpolationMode interp_);
        void setXUnchecked(int x_);
        void setX(int x_);

        static void setAnchorNodeSequence(AnchorNode *item, AnchorNodeSequence *anchorNodeSequence);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_ANCHORNODE_P_H