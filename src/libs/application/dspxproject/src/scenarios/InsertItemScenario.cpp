// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "InsertItemScenario.h"
#include "InsertItemScenario_p.h"

#include <algorithm>
#include <utility>

#include <QLoggingCategory>
#include <QPointer>
#include <QQuickWindow>
#include <QtGlobal>

#include <opendspx/track.h>

#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodelORM/Label.h>
#include <dspxmodelORM/LabelSequence.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <dspxmodelSelectionModel/TrackSelectionModel.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/Phoneme.h>

#include <dspxproject/Document.h>
#include <dspxproject/InsertItemStrategy.h>
#include <dspxproject/ProjectTimeline.h>

#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_STATIC_LOGGING_CATEGORY(lcInsertItemScenario, "diffscope.dspxproject.insertitemscenario")

    static void assignInitialTrackColor(int index, int colorCount, dspx::Track *item) {
        if (colorCount <= 0) {
            return;
        }

        const int i = index % colorCount;
        const int colorIndex = (i % 2) ? (colorCount + i) / 2 : i / 2;
        item->setColorId(colorIndex);
    }

    InsertItemScenario::InsertItemScenario(Document *document, InsertItemStrategy *strategy, const Options &options, QObject *parent)
        : QObject(parent), d_ptr(new InsertItemScenarioPrivate{.q_ptr = this, .document = document, .strategy = strategy, .options = options}) {
    }

    InsertItemScenario::~InsertItemScenario() = default;

    void InsertItemScenario::addTrack() const {
        Q_D(const InsertItemScenario);
        qCInfo(lcInsertItemScenario) << "Adding track";
        auto model = d->document->model();
        auto trackList = model->tracks();
        dspx::Track *newTrack = nullptr;
        bool success = false;
        auto selectionModel = d->document->selectionModel();
        const int colorCount = d->options.trackColorCount();
        d->document->transactionController()->beginScopedTransaction(tr("Adding track"), [=, &newTrack, &success] {
            newTrack = model->createTrack();
            newTrack->fromOpenDSPX(opendspx::Track{});
            newTrack->setName(tr("Unnamed track"));
            auto insertionIndex = trackList->size();
            assignInitialTrackColor(insertionIndex, colorCount, newTrack);
            do {
                auto currentTrack = qobject_cast<dspx::Track *>(selectionModel->currentItem());
                if (!currentTrack) {
                   break;
                }
                auto index = trackList->items().indexOf(currentTrack);
                if (index == -1) {
                   break;
                }
                insertionIndex = index + 1;
            } while (false);
            if (!trackList->insertItem(insertionIndex, newTrack)) {
                model->destroyItem(newTrack);
                newTrack = nullptr;
                return false;
            }
            success = true;
            qCDebug(lcInsertItemScenario) << "Added track" << newTrack;
            return true;
        }, [strategy = d->strategy] {
            qCWarning(lcInsertItemScenario) << "Failed to add track because the transaction could not start.";
            strategy->reportOperationConflict();
        });
        if (success && newTrack) {
            selectionModel->select(newTrack, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

    void InsertItemScenario::insertTrack(QQuickWindow *window, DocumentEdit::Hints hints) const {
        Q_D(const InsertItemScenario);
        const QPointer<Document> targetDocument(d->document);
        const QPointer<QQuickWindow> dialogWindow(window);
        Core::TransactionController::ScopedReadLock readLock(targetDocument->transactionController());
        if (!readLock) {
            qCWarning(lcInsertItemScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }

        qCInfo(lcInsertItemScenario) << "Inserting track";
        auto model = d->document->model();
        auto trackList = model->tracks();
        const auto result = d->strategy->requestTrack(trackList->size(), {trackList->size(), 1, tr("Unnamed track")}, dialogWindow, hints);
        if (!result || !readLock || !targetDocument || result->count <= 0)
            return;
        auto insertionIndex = result->index;
        const auto insertionCount = result->count;
        const auto &trackName = result->name;
        qCDebug(lcInsertItemScenario) << "Inserting track at" << insertionIndex << "with" << insertionCount << "tracks" << "named" << trackName;
        insertionIndex = std::clamp(insertionIndex, 0, trackList->size());
        QList<dspx::Track *> newTracks;
        newTracks.reserve(insertionCount);
        bool success = false;
        readLock.release();
        if (!targetDocument)
            return;
        const int colorCount = d->options.trackColorCount();
        targetDocument->transactionController()->beginScopedTransaction(tr("Inserting track"), [=, &newTracks, &success] {
            for (int i = 0; i < insertionCount; ++i) {
                auto track = model->createTrack();
                track->fromOpenDSPX(opendspx::Track{});
                track->setName(i == 0 ? trackName : QString("%1 (%L2)").arg(trackName).arg(i + 1));
                assignInitialTrackColor(insertionIndex + i, colorCount, track);
                if (!trackList->insertItem(insertionIndex + i, track)) {
                    model->destroyItem(track);
                    return false;
                }
                newTracks.append(track);
            }
            success = true;
            qCDebug(lcInsertItemScenario) << "Inserted tracks" << newTracks;
            return true;
        }, [strategy = d->strategy] {
            qCWarning(lcInsertItemScenario) << "Failed to insert track because the transaction could not start.";
            strategy->reportOperationConflict();
        });
        if (success && targetDocument && !newTracks.isEmpty()) {
            auto selectionModel = targetDocument->selectionModel();
            bool first = true;
            for (auto track : std::as_const(newTracks)) {
                auto command = dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem;
                if (first) {
                    command |= dspx::SelectionModel::ClearPreviousSelection;
                    first = false;
                }
                selectionModel->select(track, command);
            }
        }
    }

    void InsertItemScenario::insertLabel(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints) const {
        Q_D(const InsertItemScenario);
        const QPointer<Document> targetDocument(d->document);
        const QPointer<QQuickWindow> dialogWindow(window);
        Core::TransactionController::ScopedReadLock readLock(targetDocument->transactionController());
        if (!readLock) {
            qCWarning(lcInsertItemScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }

        qCInfo(lcInsertItemScenario) << "Inserting label";
        auto model = d->document->model();
        auto labelSequence = model->labels();
        const auto result = d->strategy->requestLabel({timeline->position(), {}}, timeline->musicTimeline(), dialogWindow, hints);
        if (!result || !readLock || !targetDocument)
            return;
        const auto labelPos = result->position;
        const auto &labelText = result->text;
        qCDebug(lcInsertItemScenario) << "Inserting label at" << labelPos << "with text" << labelText;
        dspx::Label *newLabel = nullptr;
        bool success = false;
        readLock.release();
        if (!targetDocument)
            return;
        targetDocument->transactionController()->beginScopedTransaction(tr("Inserting label"), [=, &newLabel, &success] {
            newLabel = model->createLabel();
            newLabel->setPosition(qMax(0, labelPos));
            newLabel->setText(labelText);
            if (!labelSequence->insertItem(newLabel)) {
                model->destroyItem(newLabel);
                newLabel = nullptr;
                return false;
            }
            success = true;
            qCDebug(lcInsertItemScenario) << "Inserted label" << newLabel;
            return true;
        }, [strategy = d->strategy] {
            qCWarning(lcInsertItemScenario) << "Failed to insert label because the transaction could not start.";
            strategy->reportOperationConflict();
        });
        if (success && targetDocument && newLabel) {
            auto selectionModel = targetDocument->selectionModel();
            selectionModel->select(newLabel, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

    void InsertItemScenario::insertSingingClip(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints) const {
        Q_D(const InsertItemScenario);

        const QPointer<Document> targetDocument(d->document);
        const QPointer<QQuickWindow> dialogWindow(window);
        Core::TransactionController::ScopedReadLock readLock(targetDocument->transactionController());
        if (!readLock) {
            qCWarning(lcInsertItemScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }

        auto model = d->document->model();
        auto trackList = model->tracks();
        if (!trackList || trackList->size() == 0)
            return;

        qCInfo(lcInsertItemScenario) << "Inserting singing clip";

        auto selectionModel = d->document->selectionModel();
        auto trackSelectionModel = selectionModel->trackSelectionModel();
        auto currentTrack = trackSelectionModel ? trackSelectionModel->currentItem() : nullptr;
        if (!currentTrack) {
            if (auto currentClip = qobject_cast<dspx::Clip *>(selectionModel ? selectionModel->currentItem() : nullptr)) {
                if (auto clipSequence = currentClip->clipSequence()) {
                    currentTrack = clipSequence->track();
                }
            }
        }

        if (!currentTrack) {
            const auto tracks = trackList->items();
            if (!tracks.isEmpty()) {
                currentTrack = tracks.first();
            }
        }

        const auto result = d->strategy->requestClip(trackList, {currentTrack, timeline->position(), 15360, tr("Unnamed clip")},
                                                     timeline->musicTimeline(), dialogWindow, hints);
        if (!result || !readLock || !targetDocument)
            return;

        auto selectedTrack = result->track;
        if (!selectedTrack) {
            selectedTrack = currentTrack;
        }
        if (!selectedTrack) {
            const auto tracks = trackList->items();
            if (!tracks.isEmpty()) {
                selectedTrack = tracks.first();
            }
        }

        if (!trackList->items().contains(selectedTrack))
            return;
        const QPointer<dspx::Track> targetTrack(selectedTrack);

        const auto clipPosition = qMax(0, result->position);
        const auto clipLength = qMax(1, result->length);
        const auto clipName = result->name;
        qCDebug(lcInsertItemScenario) << "Inserting singing clip at" << clipPosition << "with length" << clipLength << "and name" << clipName << "to track" << selectedTrack;

        dspx::SingingClip *newClip = nullptr;
        bool success = false;
        readLock.release();
        if (!targetDocument)
            return;
        targetDocument->transactionController()->beginScopedTransaction(tr("Inserting singing clip"), [=, &newClip, &success] {
            if (!targetTrack || !trackList->items().contains(targetTrack.data())) {
                return false;
            }
            newClip = model->createSingingClip();
            newClip->setName(clipName);
            newClip->setClipStart(0);
            newClip->setClipLength(clipLength);
            newClip->setPosition(clipPosition);
            newClip->setGain(1);
            auto clipSequence = targetTrack->clips();
            if (!clipSequence || !clipSequence->insertItem(newClip)) {
                model->destroyItem(newClip);
                newClip = nullptr;
                return false;
            }
            success = true;
            qCDebug(lcInsertItemScenario) << "Inserted singing clip" << newClip;
            return true;
        }, [strategy = d->strategy] {
            qCWarning(lcInsertItemScenario) << "Failed to insert singing clip because the transaction could not start.";
            strategy->reportOperationConflict();
        });

        if (success && targetDocument && newClip) {
            selectionModel->select(newClip, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

    void InsertItemScenario::insertNote(QQuickWindow *window, ProjectTimeline *timeline, dspx::SingingClip *clip, DocumentEdit::Hints hints) const {
        Q_D(const InsertItemScenario);

        const QPointer<Document> targetDocument(d->document);
        const QPointer<QQuickWindow> dialogWindow(window);
        Core::TransactionController::ScopedReadLock readLock(targetDocument->transactionController());
        if (!readLock) {
            qCWarning(lcInsertItemScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }

        auto model = d->document->model();
        auto selectionModel = d->document->selectionModel();

        dspx::NoteSequence *noteSequence = nullptr;

        // If clip is provided, use it and its notes
        if (clip) {
            if (clip->model() != model) {
                return;
            }
            noteSequence = clip->notes();
        } else {
            auto noteSelectionModel = selectionModel->noteSelectionModel();
            noteSequence = noteSelectionModel->noteSequenceWithSelectedItems();
            if (!noteSequence)
                return;
            clip = noteSequence->singingClip();
        }
        const QPointer<dspx::SingingClip> targetClip(clip);

        qCInfo(lcInsertItemScenario) << "Inserting note";

        // Calculate initial position: playback position - clip position
        const int clipPosition = clip->position();
        const int initialPosition = qMax(0, timeline->position() - clipPosition);

        const auto lyric = d->options.defaultLyricForClip(clip);
        const auto result = d->strategy->requestNote({initialPosition, 480, 60, lyric}, timeline->musicTimeline(), dialogWindow, hints);
        if (!result || !readLock || !targetDocument)
            return;

        const auto notePosition = qMax(0, result->position);
        const auto noteLength = qMax(1, result->length);
        const auto notePitch = qBound(0, result->pitch, 127);
        const auto &noteLyric = result->lyric;
        qCDebug(lcInsertItemScenario) << "Inserting note at" << notePosition << "with length" << noteLength << "and pitch" << notePitch << "and lyric" << noteLyric << "to clip" << clip;

        dspx::Note *newNote = nullptr;
        bool success = false;
        readLock.release();
        if (!targetDocument)
            return;
        targetDocument->transactionController()->beginScopedTransaction(tr("Inserting note"), [=, &newNote, &success] {
            if (!targetClip) {
                return false;
            }
            newNote = model->createNote();
            newNote->setPosition(notePosition);
            newNote->setLength(noteLength);
            newNote->setKeyNumber(notePitch);
            newNote->setLyric(noteLyric);
            if (!noteSequence->insertItem(newNote)) {
                model->destroyItem(newNote);
                newNote = nullptr;
                return false;
            }
            success = true;
            qCDebug(lcInsertItemScenario) << "Inserted note" << newNote;
            return true;
        }, [strategy = d->strategy] {
            qCWarning(lcInsertItemScenario) << "Failed to insert note because the transaction could not start.";
            strategy->reportOperationConflict();
        });

        if (success && targetDocument && newNote) {
            selectionModel->select(newNote, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

    void InsertItemScenario::insertPhoneme(QQuickWindow *window, dspx::Note *note, DocumentEdit::Hints hints) const {
        Q_D(const InsertItemScenario);

        const QPointer<Document> targetDocument(d->document);
        const QPointer<QQuickWindow> dialogWindow(window);
        Core::TransactionController::ScopedReadLock readLock(targetDocument->transactionController());
        if (!readLock) {
            qCWarning(lcInsertItemScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }

        auto model = d->document->model();
        auto selectionModel = d->document->selectionModel();

        // If note is not provided, try to get it from the selection
        if (!note) {
            if (selectionModel->selectionType() == dspx::SelectionModel::ST_Note) {
                auto noteSelectionModel = selectionModel->noteSelectionModel();
                const auto selectedNotes = noteSelectionModel->selectedItems();
                if (selectedNotes.isEmpty())
                    return;
                note = selectedNotes.first();
            } else {
                return;
            }
        }
        if (note->model() != model) {
            return;
        }
        const QPointer<dspx::Note> targetNote(note);

        qCInfo(lcInsertItemScenario) << "Inserting phoneme";

        auto phonemeSequence = note->editedPhonemes();

        const auto result = d->strategy->requestPhoneme(dialogWindow, hints);
        if (!result || !readLock || !targetDocument)
            return;

        const auto &token = result->token;
        const auto start = result->start;
        const auto &language = result->language;
        const auto onset = result->onset;
        qCDebug(lcInsertItemScenario) << "Inserting phoneme with token" << token << "start" << start << "language" << language << "onset" << onset << "to note" << note;

        dspx::Phoneme *newPhoneme = nullptr;
        bool success = false;
        readLock.release();
        if (!targetDocument)
            return;
        targetDocument->transactionController()->beginScopedTransaction(tr("Inserting phoneme"), [=, &newPhoneme, &success] {
            if (!targetNote) {
                return false;
            }
            newPhoneme = model->createPhoneme();
            newPhoneme->setToken(token);
            newPhoneme->setStart(start);
            newPhoneme->setLanguage(language);
            newPhoneme->setOnset(onset);
            if (!phonemeSequence->insertItem(newPhoneme)) {
                model->destroyItem(newPhoneme);
                newPhoneme = nullptr;
                return false;
            }
            success = true;
            qCDebug(lcInsertItemScenario) << "Inserted phoneme" << newPhoneme;
            return true;
        }, [strategy = d->strategy] {
            qCWarning(lcInsertItemScenario) << "Failed to insert phoneme because the transaction could not start.";
            strategy->reportOperationConflict();
        });
    }

}

#include "moc_InsertItemScenario.cpp"
