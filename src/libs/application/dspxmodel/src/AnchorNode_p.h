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
        AnchorNode *previousItem{};
        AnchorNode *nextItem{};

        static void setAnchorNodeSequence(AnchorNode *item, AnchorNodeSequence *anchorNodeSequence);
        static void setPreviousItem(AnchorNode *item, AnchorNode *previousItem);
        static void setNextItem(AnchorNode *item, AnchorNode *nextItem);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_ANCHORNODE_P_H