#include "ClipSelectionModel.h"
#include "ClipSelectionModel_p.h"
#include "Model.h"

#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/Track.h>

namespace dspx {

    bool ClipSelectionModelPrivate::isAddedToModel(Clip *item) const {
        return item->clipSequence() && item->clipSequence()->track()->trackList() == selectionModel->model()->tracks();
    }

    void ClipSelectionModelPrivate::updateAssociation(Clip *item) {
        Q_Q(ClipSelectionModel);
        auto previousClipSequence = clipClipSequence.value(item);
        auto currentClipSequence = item->clipSequence();
        if (previousClipSequence == currentClipSequence) {
            return;
        }
        bool clipSequencesWithSelectedItemsUpdatedFlag = false;

        if (previousClipSequence) {
            clipSequencesWithSelectedItems[previousClipSequence].remove(item);
            if (clipSequencesWithSelectedItems[previousClipSequence].isEmpty()) {
                clipSequencesWithSelectedItemsUpdatedFlag = true;
                QObject::disconnect(previousClipSequence, nullptr, q, nullptr);
                clipSequencesWithSelectedItems.remove(previousClipSequence);
            }
        }

        Q_ASSERT(currentClipSequence);
        clipClipSequence.insert(item, currentClipSequence);
        if (!clipSequencesWithSelectedItems.contains(currentClipSequence)) {
            clipSequencesWithSelectedItemsUpdatedFlag = true;
            QObject::connect(currentClipSequence->track(), &Track::trackListChanged, q, [currentClipSequence, this] {
                if (currentClipSequence->track()->trackList() == selectionModel->model()->tracks())
                    return;
                for (auto item : clipSequencesWithSelectedItems[currentClipSequence]) {
                    updateOnItemRemoved(item);
                }
            });
            QObject::connect(currentClipSequence, &QObject::destroyed, q, [currentClipSequence, this] {
                for (auto item : clipSequencesWithSelectedItems[currentClipSequence]) {
                    updateOnItemRemoved(item);
                }
            });
        }
        clipSequencesWithSelectedItems[currentClipSequence].insert(item);

        if (clipSequencesWithSelectedItemsUpdatedFlag) {
            Q_EMIT q->clipSequencesWithSelectedItemsChanged();
        }
    }

    void ClipSelectionModelPrivate::removeAssociation(Clip *item) {
        Q_Q(ClipSelectionModel);
        auto clipSequence = clipClipSequence.value(item);
        bool clipSequencesWithSelectedItemsUpdatedFlag = false;

        Q_ASSERT(clipSequence);
        clipSequencesWithSelectedItems[clipSequence].remove(item);
        if (clipSequencesWithSelectedItems[clipSequence].isEmpty()) {
            clipSequencesWithSelectedItemsUpdatedFlag = true;
            QObject::disconnect(clipSequence, nullptr, q, nullptr);
            clipSequencesWithSelectedItems.remove(clipSequence);
        }
        clipClipSequence.remove(item);

        if (clipSequencesWithSelectedItemsUpdatedFlag) {
            Q_EMIT q->clipSequencesWithSelectedItemsChanged();
        }
    }

    void ClipSelectionModelPrivate::clearAssociation() {
        Q_Q(ClipSelectionModel);
        if (clipClipSequence.isEmpty())
            return;
        clipClipSequence.clear();
        clipSequencesWithSelectedItems.clear();
        Q_EMIT q->clipSequencesWithSelectedItemsChanged();
    }

    ClipSelectionModel::ClipSelectionModel(SelectionModel *parent) : QObject(parent), d_ptr(new ClipSelectionModelPrivate) {
        Q_D(ClipSelectionModel);
        d->q_ptr = this;
        d->selectionModel = parent;
    }

    ClipSelectionModel::~ClipSelectionModel() = default;

    Clip *ClipSelectionModel::currentItem() const {
        Q_D(const ClipSelectionModel);
        return d->currentItem;
    }

    QList<Clip *> ClipSelectionModel::selectedItems() const {
        Q_D(const ClipSelectionModel);
        return d->selectedItems.values();
    }

    int ClipSelectionModel::selectedCount() const {
        Q_D(const ClipSelectionModel);
        return d->selectedItems.size();
    }

    QList<ClipSequence *> ClipSelectionModel::clipSequencesWithSelectedItems() const {
        Q_D(const ClipSelectionModel);
        return d->clipSequencesWithSelectedItems.keys();
    }

    bool ClipSelectionModel::isItemSelected(Clip *item) const {
        Q_D(const ClipSelectionModel);
        return d->selectedItems.contains(item);
    }

}

#include "moc_ClipSelectionModel.cpp"
