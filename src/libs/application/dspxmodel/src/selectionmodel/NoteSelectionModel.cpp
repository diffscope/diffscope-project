#include "NoteSelectionModel.h"
#include "NoteSelectionModel_p.h"

#include <dspxmodel/Note.h>
#include <dspxmodel/SingingClip.h>

namespace dspx {

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
        return d->selectedItems;
    }

    int NoteSelectionModel::selectedCount() const {
        Q_D(const NoteSelectionModel);
        return d->selectedItems.size();
    }

    SingingClip *NoteSelectionModel::singingClipWithSelectedItems() const {
        Q_D(const NoteSelectionModel);
        return d->singingClipWithSelectedItems;
    }

}

#include "moc_NoteSelectionModel.cpp"
