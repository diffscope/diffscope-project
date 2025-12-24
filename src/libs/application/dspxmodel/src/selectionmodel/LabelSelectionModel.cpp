#include "LabelSelectionModel.h"
#include "LabelSelectionModel_p.h"

#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Label.h>

namespace dspx {

    bool LabelSelectionModelPrivate::isAddedToModel(Label *item) const {
        return item->labelSequence() == selectionModel->model()->timeline()->labels();
    }

    LabelSelectionModel::LabelSelectionModel(SelectionModel *parent) : QObject(parent), d_ptr(new LabelSelectionModelPrivate) {
        Q_D(LabelSelectionModel);
        d->q_ptr = this;
        d->selectionModel = parent;
    }

    LabelSelectionModel::~LabelSelectionModel() = default;

    Label *LabelSelectionModel::currentItem() const {
        Q_D(const LabelSelectionModel);
        return d->currentItem;
    }

    QList<Label *> LabelSelectionModel::selectedItems() const {
        Q_D(const LabelSelectionModel);
        return d->selectedItems.values();
    }

    int LabelSelectionModel::selectedCount() const {
        Q_D(const LabelSelectionModel);
        return d->selectedItems.size();
    }

    bool LabelSelectionModel::isItemSelected(Label *item) const {
        Q_D(const LabelSelectionModel);
        return d->selectedItems.contains(item);
    }

}

#include "moc_LabelSelectionModel.cpp"
