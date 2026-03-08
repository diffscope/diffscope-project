#ifndef DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_P_H

#include <QSet>

#include <dspxmodel/TrackSelectionModel.h>
#include <dspxmodel/Track.h>

namespace dspx {

    class SelectionModel;
    class TrackList;

    class TrackSelectionModelPrivate {
        Q_DECLARE_PUBLIC(TrackSelectionModel)
    public:
        TrackSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        QSet<Track *> selectedItems;
        Track *currentItem = nullptr;
        QSet<Track *> connectedItems;

        bool isValidItem(Track *item) const;
        void connectItem(Track *item);
        void disconnectItem(Track *item);
        bool addToSelection(Track *item);
        bool removeFromSelection(Track *item);
        bool clearSelection();
        void dropItem(Track *item);
        void setCurrentItem(Track *item);

        void select(Track *item, SelectionModel::SelectionCommand command);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_P_H