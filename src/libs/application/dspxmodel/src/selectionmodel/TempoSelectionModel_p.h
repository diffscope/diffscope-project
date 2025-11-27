#ifndef DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_P_H

#include <dspxmodel/TempoSelectionModel.h>

#include <QSet>

namespace dspx {

    class TempoSelectionModelPrivate {
        Q_DECLARE_PUBLIC(TempoSelectionModel)
    public:
        TempoSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        Tempo *currentItem = nullptr;
        QSet<Tempo *> selectedItems;

        bool isAddedToModel(Tempo *item) const;
        void setCurrentItem(Tempo *item);
        void addToSelection(Tempo *item);
        void removeFromSelection(Tempo *item);
        void clearSelection();
        void updateOnItemRemoved(Tempo *item);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_P_H