#ifndef DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_P_H

#include <dspxmodel/TrackSelectionModel.h>

#include <QSet>

namespace dspx {

    class TrackSelectionModelPrivate {
        Q_DECLARE_PUBLIC(TrackSelectionModel)
    public:
        TrackSelectionModel *q_ptr;
        Track *currentItem = nullptr;
        QSet<Track *> selectedItems;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_P_H