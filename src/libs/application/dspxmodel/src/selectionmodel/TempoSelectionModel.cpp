#include "TempoSelectionModel.h"
#include "TempoSelectionModel_p.h"

#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Tempo.h>

namespace dspx {

    bool TempoSelectionModelPrivate::isAddedToModel(Tempo *item) const {
        return item->tempoSequence() == selectionModel->model()->timeline()->tempos();
    }

    TempoSelectionModel::TempoSelectionModel(SelectionModel *parent) : QObject(parent), d_ptr(new TempoSelectionModelPrivate) {
        Q_D(TempoSelectionModel);
        d->q_ptr = this;
        d->selectionModel = parent;
    }

    TempoSelectionModel::~TempoSelectionModel() = default;

    Tempo *TempoSelectionModel::currentItem() const {
        Q_D(const TempoSelectionModel);
        return d->currentItem;
    }

    QList<Tempo *> TempoSelectionModel::selectedItems() const {
        Q_D(const TempoSelectionModel);
        return d->selectedItems.values();
    }

    int TempoSelectionModel::selectedCount() const {
        Q_D(const TempoSelectionModel);
        return d->selectedItems.size();
    }

    bool TempoSelectionModel::isItemSelected(Tempo *item) const {
        Q_D(const TempoSelectionModel);
        return d->selectedItems.contains(item);
    }

}

#include "moc_TempoSelectionModel.cpp"
