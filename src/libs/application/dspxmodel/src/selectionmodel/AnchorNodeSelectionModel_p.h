#ifndef DIFFSCOPE_DSPX_MODEL_ANCHORNODESELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_ANCHORNODESELECTIONMODEL_P_H

#include <dspxmodel/AnchorNodeSelectionModel.h>

namespace dspx {

    class AnchorNodeSelectionModelPrivate {
        Q_DECLARE_PUBLIC(AnchorNodeSelectionModel)
    public:
        AnchorNodeSelectionModel *q_ptr;
        AnchorNode *currentItem = nullptr;
        QList<AnchorNode *> selectedItems;
        QList<ParamCurveAnchor *> paramCurvesAnchorWithSelectedItems;
        ParamCurveSequence *paramCurveSequenceWithSelectedItems = nullptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_ANCHORNODESELECTIONMODEL_P_H