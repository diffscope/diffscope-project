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
    void LabelSelectionModelPrivate::setCurrentItem(Label *item) {
        Q_Q(LabelSelectionModel);
        if (currentItem) {
            if (!selectedItems.contains(currentItem)) {
                QObject::disconnect(item, nullptr, q, nullptr);
            }
        }
        currentItem = item;
        if (currentItem) {
            if (!selectedItems.contains(currentItem)) {
                QObject::connect(item, &Label::labelSequenceChanged, q, [=, this] {
                    if (isAddedToModel(item))
                        return;
                    updateOnItemRemoved(item);
                });
                QObject::connect(item, &QObject::destroyed, q, [=, this] {
                    updateOnItemRemoved(item);
                });
            }
        }
    }
    void LabelSelectionModelPrivate::addToSelection(Label *item) {
        Q_Q(LabelSelectionModel);
        if (currentItem != item) {
            QObject::connect(item, &Label::labelSequenceChanged, q, [=, this] {
                if (isAddedToModel(item))
                    return;
                updateOnItemRemoved(item);
            });
            QObject::connect(item, &QObject::destroyed, q, [=, this] {
                updateOnItemRemoved(item);
            });
        }
        selectedItems.insert(item);
    }
    void LabelSelectionModelPrivate::removeFromSelection(Label *item) {
        Q_Q(LabelSelectionModel);
        if (currentItem != item) {
            QObject::disconnect(item, nullptr, q, nullptr);
        }
        selectedItems.remove(item);
    }
    void LabelSelectionModelPrivate::clearSelection() {
        Q_Q(LabelSelectionModel);
        for (auto item : selectedItems) {
            if (currentItem != item) {
                QObject::disconnect(item, nullptr, q, nullptr);
            }
        }
        selectedItems.clear();
    }
    inline void LabelSelectionModelPrivate::updateOnItemRemoved(Label *item) {
        Q_Q(LabelSelectionModel);
        bool selectionUpdatedFlag = false;
        bool currentItemUpdatedFlag = false;
        if (currentItem == item) {
            currentItemUpdatedFlag = true;
            setCurrentItem(nullptr);
        }
        if (selectedItems.contains(item)) {
            selectionUpdatedFlag = true;
            removeFromSelection(item);
        }
        if (selectionUpdatedFlag) {
            Q_EMIT q->selectedItemsChanged();
        }
        if (currentItemUpdatedFlag) {
            Q_EMIT q->currentItemChanged();
        }
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
        return QList(d->selectedItems.begin(), d->selectedItems.end());
    }

    int LabelSelectionModel::selectedCount() const {
        Q_D(const LabelSelectionModel);
        return d->selectedItems.size();
    }

    void LabelSelectionModel::select(Label *item, SelectionModel::SelectionCommand command) {
        Q_D(LabelSelectionModel);
        bool selectionUpdatedFlag = false;
        bool currentItemUpdatedFlag = false;

        if (!d->isAddedToModel(item)) {
            return;
        }

        if (command & SelectionModel::ClearPreviousSelection) {
            if (!d->selectedItems.isEmpty()) {
                selectionUpdatedFlag = true;
            }
            d->clearSelection();
        }

        if (command & SelectionModel::SetCurrentItem) {
            if (d->currentItem != item) {
                currentItemUpdatedFlag = true;
                d->setCurrentItem(item);
            }
        }

        // Update selection
        if (item) {
            if ((command & SelectionModel::Select) && (command & SelectionModel::Deselect)) {
                // Toggle
                selectionUpdatedFlag = true;
                if (d->selectedItems.contains(item)) {
                    d->removeFromSelection(item);
                } else {
                    d->addToSelection(item);
                }
            } else if (command & SelectionModel::Select) {
                // Select only
                if (!d->selectedItems.contains(item)) {
                    selectionUpdatedFlag = true;
                    d->addToSelection(item);
                }
            } else if (command & SelectionModel::Deselect) {
                // Deselect only
                if (d->selectedItems.contains(item)) {
                    selectionUpdatedFlag = true;
                    d->removeFromSelection(item);
                }
            }
        }

        // Emit signals
        if (selectionUpdatedFlag) {
            emit selectedItemsChanged();
            emit selectedCountChanged();
        }
        if (currentItemUpdatedFlag) {
            emit currentItemChanged();
        }
    }

}

#include "moc_LabelSelectionModel.cpp"
