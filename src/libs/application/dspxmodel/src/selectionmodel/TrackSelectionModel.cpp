#include "TrackSelectionModel.h"
#include "TrackSelectionModel_p.h"

#include <dspxmodel/Track.h>

namespace dspx {

    TrackSelectionModel::TrackSelectionModel(QObject *parent) : QObject(parent), d_ptr(new TrackSelectionModelPrivate) {
        Q_D(TrackSelectionModel);
        d->q_ptr = this;
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

    void TrackSelectionModel::select(Track *item, SelectionModel::SelectionCommand command) {
        Q_D(TrackSelectionModel);

    }

}

#include "moc_TrackSelectionModel.cpp"
