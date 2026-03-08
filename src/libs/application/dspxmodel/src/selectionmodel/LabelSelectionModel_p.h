#ifndef DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_P_H

#include <QSet>

#include <dspxmodel/LabelSelectionModel.h>
#include <dspxmodel/Label.h>

namespace dspx {

    class SelectionModel;
    class LabelSequence;

    class LabelSelectionModelPrivate {
        Q_DECLARE_PUBLIC(LabelSelectionModel)
    public:
        LabelSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        QSet<Label *> selectedItems;
        Label *currentItem = nullptr;
        QSet<Label *> connectedItems;

        bool isValidItem(Label *item) const;
        void connectItem(Label *item);
        void disconnectItem(Label *item);
        bool addToSelection(Label *item);
        bool removeFromSelection(Label *item);
        bool clearSelection();
        void dropItem(Label *item);
        void setCurrentItem(Label *item);

        void select(Label *item, SelectionModel::SelectionCommand command);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_P_H