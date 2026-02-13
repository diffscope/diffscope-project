#include "TempoSelectionModel.h"
#include "TempoSelectionModel_p.h"

#include <QList>

#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/TempoSequence.h>

namespace dspx {

    bool TempoSelectionModelPrivate::isValidItem(Tempo *item) const {
        return item && item->tempoSequence() == selectionModel->model()->timeline()->tempos();
    }

    void TempoSelectionModelPrivate::connectItem(Tempo *item) {
        if (connectedItems.contains(item)) {
            return;
        }
        QObject::connect(item, &QObject::destroyed, q_ptr, [this](QObject *obj) {
            dropItem(static_cast<Tempo *>(obj));
        });
        QObject::connect(item, &Tempo::tempoSequenceChanged, q_ptr, [this, item]() {
            if (!isValidItem(item)) {
                dropItem(item);
            }
        });
        connectedItems.insert(item);
    }

    void TempoSelectionModelPrivate::disconnectItem(Tempo *item) {
        QObject::disconnect(item, nullptr, q_ptr, nullptr);
        connectedItems.remove(item);
    }

    bool TempoSelectionModelPrivate::addToSelection(Tempo *item) {
        if (!isValidItem(item) || selectedItems.contains(item)) {
            return false;
        }
        connectItem(item);
        selectedItems.insert(item);
        Q_EMIT q_ptr->itemSelected(item, true);
        return true;
    }

    bool TempoSelectionModelPrivate::removeFromSelection(Tempo *item) {
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

    bool TempoSelectionModelPrivate::clearSelection() {
        if (selectedItems.isEmpty()) {
            return false;
        }
        const auto items = selectedItems.values();
        bool selectionChanged = false;
        for (auto tempo : items) {
            selectionChanged |= removeFromSelection(tempo);
        }
        return selectionChanged;
    }

    void TempoSelectionModelPrivate::dropItem(Tempo *item) {
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

    void TempoSelectionModelPrivate::setCurrentItem(Tempo *item) {
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

    void TempoSelectionModelPrivate::select(Tempo *item, SelectionModel::SelectionCommand command) {
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

#include "moc_TempoSelectionModel.cpp"
