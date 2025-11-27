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
    void TempoSelectionModelPrivate::setCurrentItem(Tempo *item) {
        Q_Q(TempoSelectionModel);
        if (currentItem) {
            if (!selectedItems.contains(currentItem)) {
                QObject::disconnect(item, nullptr, q, nullptr);
            }
        }
        currentItem = item;
        if (currentItem) {
            if (!selectedItems.contains(currentItem)) {
                QObject::connect(item, &Tempo::tempoSequenceChanged, q, [=, this] {
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
    void TempoSelectionModelPrivate::addToSelection(Tempo *item) {
        Q_Q(TempoSelectionModel);
        if (currentItem != item) {
            QObject::connect(item, &Tempo::tempoSequenceChanged, q, [=, this] {
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
    void TempoSelectionModelPrivate::removeFromSelection(Tempo *item) {
        Q_Q(TempoSelectionModel);
        if (currentItem != item) {
            QObject::disconnect(item, nullptr, q, nullptr);
        }
        selectedItems.remove(item);
    }
    void TempoSelectionModelPrivate::clearSelection() {
        Q_Q(TempoSelectionModel);
        for (auto item : selectedItems) {
            if (currentItem != item) {
                QObject::disconnect(item, nullptr, q, nullptr);
            }
        }
        selectedItems.clear();
    }
    inline void TempoSelectionModelPrivate::updateOnItemRemoved(Tempo *item) {
        Q_Q(TempoSelectionModel);
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
        return QList(d->selectedItems.begin(), d->selectedItems.end());
    }

    int TempoSelectionModel::selectedCount() const {
        Q_D(const TempoSelectionModel);
        return d->selectedItems.size();
    }

    void TempoSelectionModel::select(Tempo *item, SelectionModel::SelectionCommand command) {
        Q_D(TempoSelectionModel);
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

#include "moc_TempoSelectionModel.cpp"
