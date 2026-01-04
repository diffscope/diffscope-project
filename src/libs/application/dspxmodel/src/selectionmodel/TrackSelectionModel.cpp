#include "TrackSelectionModel.h"
#include "TrackSelectionModel_p.h"

#include <dspxmodel/Model.h>

namespace dspx {

    bool TrackSelectionModelPrivate::isAddedToModel(Track *item) const {
        return item->trackList() == selectionModel->model()->tracks();
    }

    TrackSelectionModel::TrackSelectionModel(SelectionModel *parent) : QObject(parent), d_ptr(new TrackSelectionModelPrivate) {
        Q_D(TrackSelectionModel);
        d->q_ptr = this;
        d->selectionModel = parent;
    }

    TrackSelectionModel::~TrackSelectionModel() = default;

    Track *TrackSelectionModel::currentItem() const {
        Q_D(const TrackSelectionModel);
        return d->currentItem;
    }

    QList<Track *> TrackSelectionModel::selectedItems() const {
        Q_D(const TrackSelectionModel);
        return QList(d->selectedItems.begin(), d->selectedItems.end());
    }

    int TrackSelectionModel::selectedCount() const {
        Q_D(const TrackSelectionModel);
        return d->selectedItems.size();
    }

    bool TrackSelectionModel::isItemSelected(Track *item) const {
        Q_D(const TrackSelectionModel);
        return d->selectedItems.contains(item);
    }

}

#include "moc_TrackSelectionModel.cpp"
