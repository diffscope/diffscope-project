#include "LabelSelectionModel.h"
#include "LabelSelectionModel_p.h"

#include <QList>

#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSequence.h>

namespace dspx {

    bool LabelSelectionModelPrivate::isValidItem(Label *item) const {
        return item && item->labelSequence() == selectionModel->model()->timeline()->labels();
    }

    void LabelSelectionModelPrivate::connectItem(Label *item) {
        if (connectedItems.contains(item)) {
            return;
        }
        QObject::connect(item, &QObject::destroyed, q_ptr, [this](QObject *obj) {
            dropItem(static_cast<Label *>(obj));
        });
        QObject::connect(item, &Label::labelSequenceChanged, q_ptr, [this, item]() {
            if (!isValidItem(item)) {
                dropItem(item);
            }
        });
        connectedItems.insert(item);
    }

    void LabelSelectionModelPrivate::disconnectItem(Label *item) {
        QObject::disconnect(item, nullptr, q_ptr, nullptr);
        connectedItems.remove(item);
    }

    bool LabelSelectionModelPrivate::addToSelection(Label *item) {
        if (!isValidItem(item) || selectedItems.contains(item)) {
            return false;
        }
        connectItem(item);
        selectedItems.insert(item);
        Q_EMIT q_ptr->itemSelected(item, true);
        return true;
    }

    bool LabelSelectionModelPrivate::removeFromSelection(Label *item) {
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

    bool LabelSelectionModelPrivate::clearSelection() {
        if (selectedItems.isEmpty()) {
            return false;
        }
        const auto items = selectedItems.values();
        bool selectionChanged = false;
        for (auto label : items) {
            selectionChanged |= removeFromSelection(label);
        }
        return selectionChanged;
    }

    void LabelSelectionModelPrivate::dropItem(Label *item) {
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

    void LabelSelectionModelPrivate::setCurrentItem(Label *item) {
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

    void LabelSelectionModelPrivate::select(Label *item, SelectionModel::SelectionCommand command) {
        const int oldCount = selectedItems.size();
        bool selectionChanged = false;
        if (command & SelectionModel::ClearPreviousSelection) {
            selectionChanged = selectionChanged || clearSelection();
        }
        if ((command & SelectionModel::Select) && (command & SelectionModel::Deselect)) {
            if (selectedItems.contains(item)) {
                selectionChanged = selectionChanged || removeFromSelection(item);
            } else {
                selectionChanged = selectionChanged || addToSelection(item);
            }
        } else if (command & SelectionModel::Select) {
            selectionChanged = selectionChanged || addToSelection(item);
        } else if (command & SelectionModel::Deselect) {
            selectionChanged = selectionChanged || removeFromSelection(item);
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

#include "moc_LabelSelectionModel.cpp"
