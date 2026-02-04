#ifndef DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_P_H

#include <QSet>

#include <dspxmodel/ClipSelectionModel.h>
#include <dspxmodel/Clip.h>

namespace dspx {

    class SelectionModel;
    class ClipSequence;

    class ClipSelectionModelPrivate {
        Q_DECLARE_PUBLIC(ClipSelectionModel)
    public:
        ClipSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        QSet<Clip *> selectedItems;
        Clip *currentItem = nullptr;
        QSet<Clip *> connectedItems;

        QHash<Clip *, ClipSequence *> clipToClipSequence;
        QHash<ClipSequence *, QSet<Clip *>> clipSequencesWithSelectedItems;

        bool isValidItem(Clip *item) const;
        void connectItem(Clip *item);
        void disconnectItem(Clip *item);
        bool addToSelection(Clip *item);
        bool removeFromSelection(Clip *item);
        bool clearSelection();
        void dropItem(Clip *item);
        void setCurrentItem(Clip *item);

        void select(Clip *item, SelectionModel::SelectionCommand command);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_P_H