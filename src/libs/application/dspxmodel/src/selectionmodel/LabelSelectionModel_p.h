#ifndef DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_P_H

#include <dspxmodel/LabelSelectionModel.h>

#include <QSet>

namespace dspx {

    class SelectionModel;

    class LabelSelectionModelPrivate {
        Q_DECLARE_PUBLIC(LabelSelectionModel)
    public:
        LabelSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        Label *currentItem = nullptr;
        QSet<Label *> selectedItems;

        bool isAddedToModel(Label *item) const;
        void setCurrentItem(Label *item);
        void addToSelection(Label *item);
        void removeFromSelection(Label *item);
        void clearSelection();
        inline void updateOnItemRemoved(Label *item);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_P_H