#include "NoteSelectionModel.h"
#include "NoteSelectionModel_p.h"

#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/Model.h>

namespace dspx {

    bool NoteSelectionModelPrivate::isAddedToModel(Note *item) const {
        return item->noteSequence() && item->noteSequence()->singingClip()->clipSequence() && item->noteSequence()->singingClip()->clipSequence()->track()->trackList() == selectionModel->model()->tracks();
    }
    void NoteSelectionModelPrivate::clearSuperItem() {
        Q_Q(NoteSelectionModel);
        if (superItem) {
            QObject::disconnect(superItem->singingClip(), nullptr, q, nullptr);
            superItem = nullptr;
            Q_EMIT q->noteSequenceWithSelectedItemsChanged();
        }
    }
    void NoteSelectionModelPrivate::updateSuperItem(NoteSequence *superItem_) {
        Q_Q(NoteSelectionModel);
        if (!superItem_)
            return;
        if (superItem == superItem_)
            return;
        Q_ASSERT(!superItem);
        superItem = superItem_;
        QObject::connect(superItem->singingClip(), &SingingClip::clipSequenceChanged, q, [this] {
            clearSelection();
            clearSuperItem();
        });
        Q_EMIT q->noteSequenceWithSelectedItemsChanged();
    }


    NoteSelectionModel::NoteSelectionModel(QObject *parent) : QObject(parent), d_ptr(new NoteSelectionModelPrivate) {
        Q_D(NoteSelectionModel);
        d->q_ptr = this;
    }

    NoteSelectionModel::~NoteSelectionModel() = default;

    Note *NoteSelectionModel::currentItem() const {
        Q_D(const NoteSelectionModel);
        return d->currentItem;
    }

    QList<Note *> NoteSelectionModel::selectedItems() const {
        Q_D(const NoteSelectionModel);
        return d->selectedItems.values();
    }

    int NoteSelectionModel::selectedCount() const {
        Q_D(const NoteSelectionModel);
        return d->selectedItems.size();
    }

    NoteSequence *NoteSelectionModel::noteSequenceWithSelectedItems() const {
        Q_D(const NoteSelectionModel);
        return d->superItem;
    }
    bool NoteSelectionModel::isItemSelected(Note *item) const {
        Q_D(const NoteSelectionModel);
        return d->selectedItems.contains(item);
    }

}

#include "moc_NoteSelectionModel.cpp"
