#include "ClipSelectionModel.h"
#include "ClipSelectionModel_p.h"

#include <dspxmodel/Clip.h>
#include <dspxmodel/Track.h>

namespace dspx {

    ClipSelectionModel::ClipSelectionModel(QObject *parent) : QObject(parent), d_ptr(new ClipSelectionModelPrivate) {
        Q_D(ClipSelectionModel);
        d->q_ptr = this;
    }

    ClipSelectionModel::~ClipSelectionModel() = default;

    Clip *ClipSelectionModel::currentItem() const {
        Q_D(const ClipSelectionModel);
        return d->currentItem;
    }

    QList<Clip *> ClipSelectionModel::selectedItems() const {
        Q_D(const ClipSelectionModel);
        return d->selectedItems;
    }

    int ClipSelectionModel::selectedCount() const {
        Q_D(const ClipSelectionModel);
        return d->selectedItems.size();
    }

    QList<Track *> ClipSelectionModel::tracksWithSelectedItems() const {
        Q_D(const ClipSelectionModel);
        return d->tracksWithSelectedItems;
    }

}

#include "moc_ClipSelectionModel.cpp"
