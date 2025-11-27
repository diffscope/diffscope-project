#ifndef DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_P_H

#include <dspxmodel/ClipSelectionModel.h>
namespace dspx {

    class ClipSelectionModelPrivate {
        Q_DECLARE_PUBLIC(ClipSelectionModel)
    public:
        ClipSelectionModel *q_ptr;
        Clip *currentItem = nullptr;
        QList<Clip *> selectedItems;
        QList<Track *> tracksWithSelectedItems;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_P_H