#include "TrackSelectionModel.h"
#include "TrackSelectionModel_p.h"

#include <QList>

#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>

namespace dspx {

    bool TrackSelectionModelPrivate::isValidItem(Track *item) const {
        return item && item->trackList() == selectionModel->model()->tracks();
    }

    void TrackSelectionModelPrivate::connectItem(Track *item) {
        if (connectedItems.contains(item)) {
            return;
        }
        QObject::connect(item, &QObject::destroyed, q_ptr, [this](QObject *obj) {
            dropItem(static_cast<Track *>(obj));
        });
        QObject::connect(item, &Track::trackListChanged, q_ptr, [this, item]() {
            if (!isValidItem(item)) {
                dropItem(item);
            }
        });
        connectedItems.insert(item);
    }

    void TrackSelectionModelPrivate::disconnectItem(Track *item) {
        QObject::disconnect(item, nullptr, q_ptr, nullptr);
        connectedItems.remove(item);
    }

    bool TrackSelectionModelPrivate::addToSelection(Track *item) {
        if (!isValidItem(item) || selectedItems.contains(item)) {
            return false;
        }
        connectItem(item);
        selectedItems.insert(item);
        Q_EMIT q_ptr->itemSelected(item, true);
        return true;
    }

    bool TrackSelectionModelPrivate::removeFromSelection(Track *item) {
        if (!item) {
            return false;
        }
        if (!selectedItems.remove(item)) {
            return false;
        }
        if (item != currentItem) {
            disconnectItem(item);
        }
        Q_EMIT q_ptr->itemSelected(item, false);
        return true;
    }

    bool TrackSelectionModelPrivate::clearSelection() {
        if (selectedItems.isEmpty()) {
            return false;
        }
        const auto items = selectedItems.values();
        bool selectionChanged = false;
        for (auto track : items) {
            selectionChanged |= removeFromSelection(track);
        }
        return selectionChanged;
    }

    void TrackSelectionModelPrivate::dropItem(Track *item) {
        if (!item) {
            return;
        }
        const int oldCount = selectedItems.size();
        bool selectionChanged = removeFromSelection(item);
        bool countChanged = selectionChanged && oldCount != selectedItems.size();
        bool currentChanged = false;
        if (currentItem == item) {
            if (!selectedItems.contains(item)) {
                disconnectItem(item);
            }
            currentItem = nullptr;
            currentChanged = true;
        }
        if (selectionChanged) {
            Q_EMIT q_ptr->selectedItemsChanged();
            if (countChanged) {
                Q_EMIT q_ptr->selectedCountChanged();
            }
        }
        if (currentChanged) {
            Q_EMIT q_ptr->currentItemChanged();
        }
    }

    void TrackSelectionModelPrivate::setCurrentItem(Track *item) {
        if (!isValidItem(item)) {
            item = nullptr;
        }
        if (currentItem == item) {
            return;
        }
        auto oldItem = currentItem;
        currentItem = item;
        if (oldItem && !selectedItems.contains(oldItem)) {
            disconnectItem(oldItem);
        }
        if (currentItem && !selectedItems.contains(currentItem)) {
            connectItem(currentItem);
        }
        Q_EMIT q_ptr->currentItemChanged();
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
        return d->selectedItems.values();
    }

    int TrackSelectionModel::selectedCount() const {
        Q_D(const TrackSelectionModel);
        return d->selectedItems.size();
    }

    bool TrackSelectionModel::isItemSelected(Track *item) const {
        Q_D(const TrackSelectionModel);
        return d->selectedItems.contains(item);
    }

    void TrackSelectionModelPrivate::select(Track *item, SelectionModel::SelectionCommand command) {
        const int oldCount = selectedItems.size();
        bool selectionChanged = false;
        if (command & SelectionModel::ClearPreviousSelection) {
            selectionChanged |= clearSelection();
        }
        if ((command & SelectionModel::Select) && (command & SelectionModel::Deselect)) {
            if (selectedItems.contains(item)) {
                selectionChanged |= removeFromSelection(item);
            } else {
                selectionChanged |= addToSelection(item);
            }
        } else if (command & SelectionModel::Select) {
            selectionChanged |= addToSelection(item);
        } else if (command & SelectionModel::Deselect) {
            selectionChanged |= removeFromSelection(item);
        }
        if (command & SelectionModel::SetCurrentItem) {
            setCurrentItem(item);
        }
        if (selectionChanged) {
            Q_EMIT q_ptr->selectedItemsChanged();
            if (oldCount != selectedItems.size()) {
                Q_EMIT q_ptr->selectedCountChanged();
            }
        }
    }

}

#include "moc_TrackSelectionModel.cpp"
