#include "NoteSelectionModel.h"
#include "NoteSelectionModel_p.h"

#include <QList>

#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Track.h>

namespace dspx {

    bool NoteSelectionModelPrivate::isValidItem(Note *item) const {
        if (!item) {
            return false;
        }
        auto noteSeq = item->noteSequence();
        if (!noteSeq) {
            return false;
        }
        // noteSeq->singingClip() is guaranteed to be non-null (CONSTANT property)
        auto clipSeq = noteSeq->singingClip()->clipSequence();
        if (!clipSeq) {
            return false;
        }
        // clipSeq->track() is guaranteed to be non-null (CONSTANT property)
        auto trackList = clipSeq->track()->trackList();
        if (!trackList) {
            return false;
        }
        return trackList == selectionModel->model()->tracks();
    }

    void NoteSelectionModelPrivate::connectItem(Note *item) {
        if (connectedItems.contains(item)) {
            return;
        }
        QObject::connect(item, &QObject::destroyed, q_ptr, [this](QObject *obj) {
            dropItem(static_cast<Note *>(obj));
        });
        QObject::connect(item, &Note::noteSequenceChanged, q_ptr, [this, item]() {
            auto newNoteSeq = item->noteSequence();
            if (newNoteSeq != noteSequenceWithSelectedItems) {
                // Note has moved to a different NoteSequence
                dropItem(item);
            }
        });
        connectedItems.insert(item);
    }

    void NoteSelectionModelPrivate::disconnectItem(Note *item) {
        QObject::disconnect(item, nullptr, q_ptr, nullptr);
        connectedItems.remove(item);
    }

    bool NoteSelectionModelPrivate::addToSelection(Note *item) {
        if (!isValidItem(item) || selectedItems.contains(item)) {
            return false;
        }
        connectItem(item);
        selectedItems.insert(item);
        Q_EMIT q_ptr->itemSelected(item, true);
        return true;
    }

    bool NoteSelectionModelPrivate::removeFromSelection(Note *item) {
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

    bool NoteSelectionModelPrivate::clearSelection() {
        if (selectedItems.isEmpty()) {
            return false;
        }
        const auto items = selectedItems.values();
        bool selectionChanged = false;
        for (auto note : items) {
            selectionChanged |= removeFromSelection(note);
        }
        return selectionChanged;
    }

    void NoteSelectionModelPrivate::dropItem(Note *item) {
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

    void NoteSelectionModelPrivate::setCurrentItem(Note *item) {
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

    void NoteSelectionModelPrivate::connectNoteSequence(NoteSequence *noteSeq) {
        if (!noteSeq) {
            return;
        }

        // Connect to NoteSequence destroyed signal
        QObject::connect(noteSeq, &QObject::destroyed, q_ptr, [this]() {
            clearSelection();
            setCurrentItem(nullptr);
            noteSequenceWithSelectedItems = nullptr;
            Q_EMIT q_ptr->noteSequenceWithSelectedItemsChanged();
        });

        // noteSeq->singingClip() is guaranteed to be non-null
        auto singingClip = noteSeq->singingClip();

        // Connect to SingingClip::clipSequenceChanged
        QObject::connect(singingClip, &SingingClip::clipSequenceChanged, q_ptr, [this, noteSeq]() {
            auto clipSeq = noteSeq->singingClip()->clipSequence();
            
            if (!clipSeq) {
                // SingingClip has been detached from ClipSequence
                clearAllAndResetNoteSequence();
                return;
            }
            
            // clipSeq->track() is guaranteed to be non-null
            auto track = clipSeq->track();
            
            // Reconnect to Track::trackListChanged
            // Note: We need to disconnect old track connections first
            // This is handled by disconnectNoteSequence() being called before connectNoteSequence()
            QObject::connect(track, &Track::trackListChanged, q_ptr, [this, noteSeq]() {
                // Get current trackList through the full chain
                auto clipSeq = noteSeq->singingClip()->clipSequence();
                if (!clipSeq) {
                    clearAllAndResetNoteSequence();
                    return;
                }
                auto trackList = clipSeq->track()->trackList();
                
                if (trackList != selectionModel->model()->tracks()) {
                    // NoteSequence has left the current Model
                    clearAllAndResetNoteSequence();
                }
            });
        });

        // Also connect to Track::trackListChanged initially
        auto clipSeq = singingClip->clipSequence();
        if (clipSeq) {
            auto track = clipSeq->track();
            QObject::connect(track, &Track::trackListChanged, q_ptr, [this, noteSeq]() {
                // Get current trackList through the full chain
                auto clipSeq = noteSeq->singingClip()->clipSequence();
                if (!clipSeq) {
                    clearAllAndResetNoteSequence();
                    return;
                }
                auto trackList = clipSeq->track()->trackList();
                
                if (trackList != selectionModel->model()->tracks()) {
                    // NoteSequence has left the current Model
                    clearAllAndResetNoteSequence();
                }
            });
        }
    }

    void NoteSelectionModelPrivate::disconnectNoteSequence() {
        if (!noteSequenceWithSelectedItems) {
            return;
        }

        // Disconnect from NoteSequence
        QObject::disconnect(noteSequenceWithSelectedItems, nullptr, q_ptr, nullptr);

        // Disconnect from SingingClip
        auto singingClip = noteSequenceWithSelectedItems->singingClip();
        if (singingClip) {
            QObject::disconnect(singingClip, nullptr, q_ptr, nullptr);
        }

        // Disconnect from Track (if clipSequence exists)
        auto clipSeq = singingClip ? singingClip->clipSequence() : nullptr;
        if (clipSeq) {
            auto track = clipSeq->track();
            if (track) {
                QObject::disconnect(track, nullptr, q_ptr, nullptr);
            }
        }
    }

    void NoteSelectionModelPrivate::clearAllAndResetNoteSequence() {
        clearSelection();
        setCurrentItem(nullptr);
        disconnectNoteSequence();
        noteSequenceWithSelectedItems = nullptr;
        Q_EMIT q_ptr->noteSequenceWithSelectedItemsChanged();
    }

    void NoteSelectionModelPrivate::select(Note *item, SelectionModel::SelectionCommand command, NoteSequence *containerItemHint) {
        const int oldCount = selectedItems.size();
        bool selectionChanged = false;
        bool noteSequenceChanged = false;

        // Handle ClearPreviousSelection
        if (command & SelectionModel::ClearPreviousSelection) {
            selectionChanged |= clearSelection();
        }

        // Handle item == nullptr case with containerItemHint
        if (!item && containerItemHint) {
            // Validate containerItemHint
            bool containerValid = false;
            if (containerItemHint->singingClip()) {
                auto clipSeq = containerItemHint->singingClip()->clipSequence();
                if (clipSeq && clipSeq->track()) {
                    auto trackList = clipSeq->track()->trackList();
                    containerValid = (trackList == selectionModel->model()->tracks());
                }
            }

            if (containerValid && noteSequenceWithSelectedItems != containerItemHint) {
                disconnectNoteSequence();
                noteSequenceWithSelectedItems = containerItemHint;
                connectNoteSequence(noteSequenceWithSelectedItems);
                noteSequenceChanged = true;
            }
        }

        // Handle item selection
        if (item) {
            if (!isValidItem(item)) {
                // Invalid item, ignore
                return;
            }

            auto itemNoteSeq = item->noteSequence();

            // Check if we need to change noteSequenceWithSelectedItems
            if (!noteSequenceWithSelectedItems) {
                // Case A: Set noteSequenceWithSelectedItems for the first time
                noteSequenceWithSelectedItems = itemNoteSeq;
                connectNoteSequence(noteSequenceWithSelectedItems);
                noteSequenceChanged = true;
            } else if (noteSequenceWithSelectedItems != itemNoteSeq) {
                // Case C: Different NoteSequence, clear and switch
                selectionChanged |= clearSelection();
                if (currentItem) {
                    setCurrentItem(nullptr);
                }
                disconnectNoteSequence();
                noteSequenceWithSelectedItems = itemNoteSeq;
                connectNoteSequence(noteSequenceWithSelectedItems);
                noteSequenceChanged = true;
            }
            // Case B: Same NoteSequence, proceed with selection

            // Execute selection command
            if ((command & SelectionModel::Select) && (command & SelectionModel::Deselect)) {
                // Toggle
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
        } else {
            // item is nullptr and no containerItemHint or containerItemHint is invalid
            if (command & SelectionModel::SetCurrentItem) {
                setCurrentItem(nullptr);
            }
        }

        // Emit signals
        if (selectionChanged) {
            Q_EMIT q_ptr->selectedItemsChanged();
            if (oldCount != selectedItems.size()) {
                Q_EMIT q_ptr->selectedCountChanged();
            }
        }
        if (noteSequenceChanged) {
            Q_EMIT q_ptr->noteSequenceWithSelectedItemsChanged();
        }
    }

    NoteSelectionModel::NoteSelectionModel(QObject *parent) : QObject(parent), d_ptr(new NoteSelectionModelPrivate) {
        Q_D(NoteSelectionModel);
        d->q_ptr = this;
        d->selectionModel = qobject_cast<SelectionModel *>(parent);
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
        return d->noteSequenceWithSelectedItems;
    }

    bool NoteSelectionModel::isItemSelected(Note *item) const {
        Q_D(const NoteSelectionModel);
        return d->selectedItems.contains(item);
    }

}

#include "moc_NoteSelectionModel.cpp"
