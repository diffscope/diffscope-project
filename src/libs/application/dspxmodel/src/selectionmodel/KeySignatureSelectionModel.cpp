#include "KeySignatureSelectionModel.h"
#include "KeySignatureSelectionModel_p.h"

#include <QList>

#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/KeySignature.h>
#include <dspxmodel/KeySignatureSequence.h>

namespace dspx {

    bool KeySignatureSelectionModelPrivate::isValidItem(KeySignature *item) const {
        return item && item->keySignatureSequence() == selectionModel->model()->timeline()->keySignatures();
    }

    void KeySignatureSelectionModelPrivate::connectItem(KeySignature *item) {
        if (connectedItems.contains(item)) {
            return;
        }
        QObject::connect(item, &QObject::destroyed, q_ptr, [this](QObject *obj) {
            dropItem(static_cast<KeySignature *>(obj));
        });
        QObject::connect(item, &KeySignature::keySignatureSequenceChanged, q_ptr, [this, item]() {
            if (!isValidItem(item)) {
                dropItem(item);
            }
        });
        connectedItems.insert(item);
    }

    void KeySignatureSelectionModelPrivate::disconnectItem(KeySignature *item) {
        QObject::disconnect(item, nullptr, q_ptr, nullptr);
        connectedItems.remove(item);
    }

    bool KeySignatureSelectionModelPrivate::addToSelection(KeySignature *item) {
        if (!isValidItem(item) || selectedItems.contains(item)) {
            return false;
        }
        connectItem(item);
        selectedItems.insert(item);
        Q_EMIT q_ptr->itemSelected(item, true);
        return true;
    }

    bool KeySignatureSelectionModelPrivate::removeFromSelection(KeySignature *item) {
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

    bool KeySignatureSelectionModelPrivate::clearSelection() {
        if (selectedItems.isEmpty()) {
            return false;
        }
        const auto items = selectedItems.values();
        bool selectionChanged = false;
        for (auto keySignature : items) {
            selectionChanged |= removeFromSelection(keySignature);
        }
        return selectionChanged;
    }

    void KeySignatureSelectionModelPrivate::dropItem(KeySignature *item) {
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

    void KeySignatureSelectionModelPrivate::setCurrentItem(KeySignature *item) {
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

    KeySignatureSelectionModel::KeySignatureSelectionModel(SelectionModel *parent) : QObject(parent), d_ptr(new KeySignatureSelectionModelPrivate) {
        Q_D(KeySignatureSelectionModel);
        d->q_ptr = this;
        d->selectionModel = parent;
    }

    KeySignatureSelectionModel::~KeySignatureSelectionModel() = default;

    KeySignature *KeySignatureSelectionModel::currentItem() const {
        Q_D(const KeySignatureSelectionModel);
        return d->currentItem;
    }

    QList<KeySignature *> KeySignatureSelectionModel::selectedItems() const {
        Q_D(const KeySignatureSelectionModel);
        return d->selectedItems.values();
    }

    int KeySignatureSelectionModel::selectedCount() const {
        Q_D(const KeySignatureSelectionModel);
        return d->selectedItems.size();
    }

    bool KeySignatureSelectionModel::isItemSelected(KeySignature *item) const {
        Q_D(const KeySignatureSelectionModel);
        return d->selectedItems.contains(item);
    }

    void KeySignatureSelectionModelPrivate::select(KeySignature *item, SelectionModel::SelectionCommand command) {
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

#include "moc_KeySignatureSelectionModel.cpp"
