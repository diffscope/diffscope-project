#ifndef DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_P_H

#include <QSet>

#include <dspxmodel/TempoSelectionModel.h>
#include <dspxmodel/Tempo.h>

namespace dspx {

    class SelectionModel;
    class TempoSequence;

    class TempoSelectionModelPrivate {
        Q_DECLARE_PUBLIC(TempoSelectionModel)
    public:
        TempoSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        QSet<Tempo *> selectedItems;
        Tempo *currentItem = nullptr;
        QSet<Tempo *> connectedItems;

        bool isValidItem(Tempo *item) const;
        void connectItem(Tempo *item);
        void disconnectItem(Tempo *item);
        bool addToSelection(Tempo *item);
        bool removeFromSelection(Tempo *item);
        bool clearSelection();
        void dropItem(Tempo *item);
        void setCurrentItem(Tempo *item);

        void select(Tempo *item, SelectionModel::SelectionCommand command);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_P_H