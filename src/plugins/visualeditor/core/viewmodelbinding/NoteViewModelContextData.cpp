#include "NoteViewModelContextData_p.h"

#include <QLoggingCategory>
#include <QQuickItem>
#include <QState>
#include <QStateMachine>

#include <ScopicFlowCore/NoteEditLayerInteractionController.h>
#include <ScopicFlowCore/NoteViewModel.h>
#include <ScopicFlowCore/RangeSequenceViewModel.h>
#include <ScopicFlowCore/ClipViewModel.h>

#include <opendspx/note.h>

#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/Pronunciation.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>
#include <visualeditor/private/NoteSelectionController_p.h>

namespace VisualEditor {

    namespace {
        class NoteEditLayerInteractionControllerProxy : public sflow::NoteEditLayerInteractionController {
        public:
            explicit NoteEditLayerInteractionControllerProxy(NoteViewModelContextData *context, QObject *parent)
                : sflow::NoteEditLayerInteractionController(parent), m_context(context) {
            }

            sflow::NoteViewModel *createAndInsertNoteOnDrawing(sflow::RangeSequenceViewModel *noteSequenceViewModel, int position, int trackIndex) override;

        private:
            NoteViewModelContextData *m_context{};
        };

        inline QString pronunciationAdditionalText(const dspx::Pronunciation *pronunciation) {
            if (!pronunciation) {
                return {};
            }
            return pronunciation->edited().isEmpty() ? pronunciation->original() : pronunciation->edited();
        }

        inline dspx::Note *duplicateNote(dspx::Note *source, dspx::Model *model) {
            if (!source || !model) {
                return nullptr;
            }

            auto duplicated = model->createNote();
            duplicated->fromQDspx(source->toQDspx());
            return duplicated;
        }
    }

    Q_STATIC_LOGGING_CATEGORY(lcNoteViewModelContextData, "diffscope.visualeditor.noteviewmodelcontextdata")

    void NoteViewModelContextData::initStateMachine() {
        stateMachine = new QStateMachine(QState::ExclusiveStates, this);

        idleState = new QState;
        rubberBandDraggingState = new QState;

        movePendingState = new QState;
        moveProcessingState = new QState;
        moveCommittingState = new QState;
        moveAbortingState = new QState;

        adjustPendingState = new QState;
        adjustProcessingState = new QState;
        adjustCommittingState = new QState;
        adjustAbortingState = new QState;

        drawPendingState = new QState;
        drawProcessingState = new QState;
        drawCommittingState = new QState;
        drawAbortingState = new QState;

        splitPendingState = new QState;
        splitCommittingState = new QState;

        lyricPendingState = new QState;
        lyricProgressingState = new QState;
        lyricCommittingState = new QState;
        lyricAbortingState = new QState;

        additionalTextPendingState = new QState;
        additionalTextProgressingState = new QState;
        additionalTextCommittingState = new QState;
        additionalTextAbortingState = new QState;

        stateMachine->addState(idleState);
        stateMachine->addState(rubberBandDraggingState);
        stateMachine->addState(movePendingState);
        stateMachine->addState(moveProcessingState);
        stateMachine->addState(moveCommittingState);
        stateMachine->addState(moveAbortingState);
        stateMachine->addState(adjustPendingState);
        stateMachine->addState(adjustProcessingState);
        stateMachine->addState(adjustCommittingState);
        stateMachine->addState(adjustAbortingState);
        stateMachine->addState(drawPendingState);
        stateMachine->addState(drawProcessingState);
        stateMachine->addState(drawCommittingState);
        stateMachine->addState(drawAbortingState);
        stateMachine->addState(splitPendingState);
        stateMachine->addState(splitCommittingState);
        stateMachine->addState(lyricPendingState);
        stateMachine->addState(lyricProgressingState);
        stateMachine->addState(lyricCommittingState);
        stateMachine->addState(lyricAbortingState);
        stateMachine->addState(additionalTextPendingState);
        stateMachine->addState(additionalTextProgressingState);
        stateMachine->addState(additionalTextCommittingState);
        stateMachine->addState(additionalTextAbortingState);
        stateMachine->setInitialState(idleState);
        stateMachine->start();

        idleState->addTransition(this, &NoteViewModelContextData::rubberBandDragWillStart, rubberBandDraggingState);
        rubberBandDraggingState->addTransition(this, &NoteViewModelContextData::rubberBandDragWillFinish, idleState);

        idleState->addTransition(this, &NoteViewModelContextData::moveTransactionWillStart, movePendingState);
        movePendingState->addTransition(this, &NoteViewModelContextData::moveTransactionStarted, moveProcessingState);
        movePendingState->addTransition(this, &NoteViewModelContextData::moveTransactionNotStarted, idleState);
        moveProcessingState->addTransition(this, &NoteViewModelContextData::moveTransactionWillCommit, moveCommittingState);
        moveProcessingState->addTransition(this, &NoteViewModelContextData::moveTransactionWillAbort, moveAbortingState);
        moveCommittingState->addTransition(idleState);
        moveAbortingState->addTransition(idleState);

        idleState->addTransition(this, &NoteViewModelContextData::adjustTransactionWillStart, adjustPendingState);
        adjustPendingState->addTransition(this, &NoteViewModelContextData::adjustTransactionStarted, adjustProcessingState);
        adjustPendingState->addTransition(this, &NoteViewModelContextData::adjustTransactionNotStarted, idleState);
        adjustProcessingState->addTransition(this, &NoteViewModelContextData::adjustTransactionWillCommit, adjustCommittingState);
        adjustProcessingState->addTransition(this, &NoteViewModelContextData::adjustTransactionWillAbort, adjustAbortingState);
        adjustCommittingState->addTransition(idleState);
        adjustAbortingState->addTransition(idleState);

        idleState->addTransition(this, &NoteViewModelContextData::drawTransactionWillStart, drawPendingState);
        drawPendingState->addTransition(this, &NoteViewModelContextData::drawTransactionStarted, drawProcessingState);
        drawPendingState->addTransition(this, &NoteViewModelContextData::drawTransactionNotStarted, idleState);
        drawProcessingState->addTransition(this, &NoteViewModelContextData::drawTransactionWillCommit, drawCommittingState);
        drawProcessingState->addTransition(this, &NoteViewModelContextData::drawTransactionWillAbort, drawAbortingState);
        drawCommittingState->addTransition(idleState);
        drawAbortingState->addTransition(idleState);

        idleState->addTransition(this, &NoteViewModelContextData::splitWillStart, splitPendingState);
        splitPendingState->addTransition(this, &NoteViewModelContextData::splitWillCommit, splitCommittingState);
        splitPendingState->addTransition(this, &NoteViewModelContextData::splitWillAbort, idleState);
        splitCommittingState->addTransition(idleState);

        idleState->addTransition(this, &NoteViewModelContextData::lyricTransactionWillStart, lyricPendingState);
        lyricPendingState->addTransition(this, &NoteViewModelContextData::lyricTransactionStarted, lyricProgressingState);
        lyricPendingState->addTransition(this, &NoteViewModelContextData::lyricTransactionNotStarted, idleState);
        lyricProgressingState->addTransition(this, &NoteViewModelContextData::lyricTransactionWillCommit, lyricCommittingState);
        lyricProgressingState->addTransition(this, &NoteViewModelContextData::lyricTransactionWillAbort, lyricAbortingState);
        lyricCommittingState->addTransition(idleState);
        lyricAbortingState->addTransition(idleState);

        idleState->addTransition(this, &NoteViewModelContextData::additionalTextTransactionWillStart, additionalTextPendingState);
        additionalTextPendingState->addTransition(this, &NoteViewModelContextData::additionalTextTransactionStarted, additionalTextProgressingState);
        additionalTextPendingState->addTransition(this, &NoteViewModelContextData::additionalTextTransactionNotStarted, idleState);
        additionalTextProgressingState->addTransition(this, &NoteViewModelContextData::additionalTextTransactionWillCommit, additionalTextCommittingState);
        additionalTextProgressingState->addTransition(this, &NoteViewModelContextData::additionalTextTransactionWillAbort, additionalTextAbortingState);
        additionalTextCommittingState->addTransition(idleState);
        additionalTextAbortingState->addTransition(idleState);

        auto logEntered = [](const char *name) { qCInfo(lcNoteViewModelContextData) << name << "entered"; };
        auto logExited = [](const char *name) { qCInfo(lcNoteViewModelContextData) << name << "exited"; };

        connect(idleState, &QState::entered, this, [=] { logEntered("Idle state"); });
        connect(idleState, &QState::exited, this, [=] { logExited("Idle state"); });
        connect(rubberBandDraggingState, &QState::entered, this, [=] { logEntered("Rubber band dragging state"); });
        connect(rubberBandDraggingState, &QState::exited, this, [=] { logExited("Rubber band dragging state"); });

        connect(movePendingState, &QState::entered, this, [=, this] {
            logEntered("Move pending state");
            onMovePendingStateEntered();
        });
        connect(movePendingState, &QState::exited, this, [=] { logExited("Move pending state"); });
        connect(moveProcessingState, &QState::entered, this, [=] { logEntered("Move processing state"); });
        connect(moveProcessingState, &QState::exited, this, [=] { logExited("Move processing state"); });
        connect(moveCommittingState, &QState::entered, this, [=, this] {
            logEntered("Move committing state");
            onMoveCommittingStateEntered();
        });
        connect(moveCommittingState, &QState::exited, this, [=] { logExited("Move committing state"); });
        connect(moveAbortingState, &QState::entered, this, [=, this] {
            logEntered("Move aborting state");
            onMoveAbortingStateEntered();
        });
        connect(moveAbortingState, &QState::exited, this, [=] { logExited("Move aborting state"); });

        connect(adjustPendingState, &QState::entered, this, [=, this] {
            logEntered("Adjust pending state");
            onAdjustPendingStateEntered();
        });
        connect(adjustPendingState, &QState::exited, this, [=] { logExited("Adjust pending state"); });
        connect(adjustProcessingState, &QState::entered, this, [=] { logEntered("Adjust processing state"); });
        connect(adjustProcessingState, &QState::exited, this, [=] { logExited("Adjust processing state"); });
        connect(adjustCommittingState, &QState::entered, this, [=, this] {
            logEntered("Adjust committing state");
            onAdjustCommittingStateEntered();
        });
        connect(adjustCommittingState, &QState::exited, this, [=] { logExited("Adjust committing state"); });
        connect(adjustAbortingState, &QState::entered, this, [=, this] {
            logEntered("Adjust aborting state");
            onAdjustAbortingStateEntered();
        });
        connect(adjustAbortingState, &QState::exited, this, [=] { logExited("Adjust aborting state"); });

        connect(drawPendingState, &QState::entered, this, [=, this] {
            logEntered("Draw pending state");
            onDrawPendingStateEntered();
        });
        connect(drawPendingState, &QState::exited, this, [=] { logExited("Draw pending state"); });
        connect(drawProcessingState, &QState::entered, this, [=] { logEntered("Draw processing state"); });
        connect(drawProcessingState, &QState::exited, this, [=] { logExited("Draw processing state"); });
        connect(drawCommittingState, &QState::entered, this, [=, this] {
            logEntered("Draw committing state");
            onDrawCommittingStateEntered();
        });
        connect(drawCommittingState, &QState::exited, this, [=] { logExited("Draw committing state"); });
        connect(drawAbortingState, &QState::entered, this, [=, this] {
            logEntered("Draw aborting state");
            onDrawAbortingStateEntered();
        });
        connect(drawAbortingState, &QState::exited, this, [=] { logExited("Draw aborting state"); });

        connect(splitPendingState, &QState::entered, this, [=] { logEntered("Split pending state"); });
        connect(splitPendingState, &QState::exited, this, [=] { logExited("Split pending state"); });
        connect(splitCommittingState, &QState::entered, this, [=, this] {
            logEntered("Split committing state");
            onSplitCommittingStateEntered();
        });
        connect(splitCommittingState, &QState::exited, this, [=] { logExited("Split committing state"); });

        connect(lyricPendingState, &QState::entered, this, [=, this] {
            logEntered("Lyric pending state");
            onLyricPendingStateEntered();
        });
        connect(lyricPendingState, &QState::exited, this, [=] { logExited("Lyric pending state"); });
        connect(lyricProgressingState, &QState::entered, this, [=] { logEntered("Lyric progressing state"); });
        connect(lyricProgressingState, &QState::exited, this, [=] { logExited("Lyric progressing state"); });
        connect(lyricCommittingState, &QState::entered, this, [=, this] {
            logEntered("Lyric committing state");
            onLyricCommittingStateEntered();
        });
        connect(lyricCommittingState, &QState::exited, this, [=] { logExited("Lyric committing state"); });
        connect(lyricAbortingState, &QState::entered, this, [=, this] {
            logEntered("Lyric aborting state");
            onLyricAbortingStateEntered();
        });
        connect(lyricAbortingState, &QState::exited, this, [=] { logExited("Lyric aborting state"); });

        connect(additionalTextPendingState, &QState::entered, this, [=, this] {
            logEntered("Additional text pending state");
            onAdditionalTextPendingStateEntered();
        });
        connect(additionalTextPendingState, &QState::exited, this, [=] { logExited("Additional text pending state"); });
        connect(additionalTextProgressingState, &QState::entered, this, [=] { logEntered("Additional text progressing state"); });
        connect(additionalTextProgressingState, &QState::exited, this, [=] { logExited("Additional text progressing state"); });
        connect(additionalTextCommittingState, &QState::entered, this, [=, this] {
            logEntered("Additional text committing state");
            onAdditionalTextCommittingStateEntered();
        });
        connect(additionalTextCommittingState, &QState::exited, this, [=] { logExited("Additional text committing state"); });
        connect(additionalTextAbortingState, &QState::entered, this, [=, this] {
            logEntered("Additional text aborting state");
            onAdditionalTextAbortingStateEntered();
        });
        connect(additionalTextAbortingState, &QState::exited, this, [=] { logExited("Additional text aborting state"); });
    }

    void NoteViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        document = q->windowHandle()->projectDocumentContext()->document();
        selectionModel = document->selectionModel();
        trackList = document->model()->tracks();
        noteSelectionModel = selectionModel->noteSelectionModel();
        noteSelectionController = new NoteSelectionController(q);

        initStateMachine();
    }

    void NoteViewModelContextData::bindTrackSequences() {
        Q_Q(ProjectViewModelContext);
        connect(trackList, &dspx::TrackList::itemInserted, this, [=, this](int, dspx::Track *track) {
            bindTrack(track);
        });
        connect(trackList, &dspx::TrackList::itemRemoved, this, [=, this](int, dspx::Track *track) {
            unbindTrack(track);
        });
        connect(trackList, &dspx::TrackList::rotated, this, [=, this](int, int, int) {
            for (auto clip : noteSequenceViewModelMap.keys()) {
                auto track = clip->clipSequence()->track();
                auto trackViewModel = singingClipPerTrackSequenceViewModelMap.value(track);
                if (trackViewModel && !trackViewModel->items().contains(q->getClipViewItemFromDocumentItem(clip))) {
                    trackViewModel->insertItem(q->getClipViewItemFromDocumentItem(clip));
                }
            }
        });

        for (auto track : trackList->items()) {
            bindTrack(track);
        }

        connect(noteSelectionModel, &dspx::NoteSelectionModel::itemSelected, this, [=, this](dspx::Note *item, bool selected) {
            auto viewItem = noteViewItemMap.value(item);
            Q_ASSERT(viewItem);
            viewItem->setSelected(selected);
        });

        for (auto selectedItem : noteSelectionModel->selectedItems()) {
            auto viewItem = noteViewItemMap.value(selectedItem);
            if (viewItem) {
                viewItem->setSelected(true);
            }
        }
    }

    void NoteViewModelContextData::bindTrack(dspx::Track *track) {
        Q_Q(ProjectViewModelContext);
        auto sequenceViewModel = new sflow::RangeSequenceViewModel(q_ptr, "position", "length");
        singingClipPerTrackSequenceViewModelMap.insert(track, sequenceViewModel);
        trackFromSingingClipSequenceViewModelMap.insert(sequenceViewModel, track);

        auto sequence = track->clips();
        connect(sequence, &dspx::ClipSequence::itemInserted, this, [=, this](dspx::Clip *clip) {
            auto singing = qobject_cast<dspx::SingingClip *>(clip);
            if (singing) {
                bindSingingClip(singing);
            }
        });
        connect(sequence, &dspx::ClipSequence::itemRemoved, this, [=, this](dspx::Clip *clip, dspx::ClipSequence *clipSequenceToWhichMoved) {
            auto singing = qobject_cast<dspx::SingingClip *>(clip);
            if (!singing) {
                return;
            }
            if (clipSequenceToWhichMoved && trackList->items().contains(clipSequenceToWhichMoved->track())) {
                return;
            }
            unbindSingingClip(singing);
        });

        for (auto clip : sequence->asRange()) {
            if (clip->type() == dspx::Clip::Singing) {
                bindSingingClip(static_cast<dspx::SingingClip *>(clip));
            }
        }
    }

    void NoteViewModelContextData::unbindTrack(dspx::Track *track) {
        auto sequenceViewModel = singingClipPerTrackSequenceViewModelMap.take(track);
        trackFromSingingClipSequenceViewModelMap.remove(sequenceViewModel);
        if (sequenceViewModel) {
            sequenceViewModel->deleteLater();
        }

        auto sequence = track->clips();
        disconnect(sequence, nullptr, this, nullptr);
        for (auto clip : noteSequenceViewModelMap.keys()) {
            if (clip->clipSequence() && clip->clipSequence()->track() == track) {
                unbindSingingClip(clip);
            }
        }
    }

    void NoteViewModelContextData::bindSingingClip(dspx::SingingClip *clip) {
        Q_Q(ProjectViewModelContext);
        if (noteSequenceViewModelMap.contains(clip)) {
            return;
        }

        auto viewItem = q_ptr->getClipViewItemFromDocumentItem(clip);
        Q_ASSERT(viewItem);

        auto track = clip->clipSequence()->track();
        auto trackSequenceViewModel = singingClipPerTrackSequenceViewModelMap.value(track);
        Q_ASSERT(trackSequenceViewModel);
        if (!trackSequenceViewModel->items().contains(viewItem)) {
            trackSequenceViewModel->insertItem(viewItem);
        }

        auto noteSequenceViewModel = new sflow::RangeSequenceViewModel(trackSequenceViewModel);
        noteSequenceViewModelMap.insert(clip, noteSequenceViewModel);
        singingClipFromNoteSequenceViewModelMap.insert(noteSequenceViewModel, clip);
        viewItem->setAssociatedNoteSequence(noteSequenceViewModel);

        auto noteSequence = clip->notes();
        connect(noteSequence, &dspx::NoteSequence::itemInserted, this, [=, this](dspx::Note *item) {
            bindNoteDocumentItem(item);
        });
        connect(noteSequence, &dspx::NoteSequence::itemRemoved, this, [=, this](dspx::Note *item) {
            unbindNoteDocumentItem(item);
        });

        connect(clip, &dspx::Clip::clipSequenceChanged, this, [=, this] {
            // FIXME why to iterate all singingClipPerTrackSequenceViewModel?
            for (auto sequenceViewModel : singingClipPerTrackSequenceViewModelMap.values()) {
                sequenceViewModel->removeItem(viewItem);
            }
            if (clip->clipSequence()) {
                auto currentTrack = clip->clipSequence()->track();
                auto targetSequenceViewModel = singingClipPerTrackSequenceViewModelMap.value(currentTrack);
                Q_ASSERT(targetSequenceViewModel);
                targetSequenceViewModel->insertItem(viewItem);
            }
        });

        for (auto note : noteSequence->asRange()) {
            bindNoteDocumentItem(note);
        }
    }

    void NoteViewModelContextData::unbindSingingClip(dspx::SingingClip *clip) {
        if (!noteSequenceViewModelMap.contains(clip)) {
            return;
        }

        auto noteSequence = clip->notes();
        disconnect(noteSequence, nullptr, this, nullptr);
        for (auto note : noteSequence->asRange()) {
            unbindNoteDocumentItem(note);
        }

        if (clip->clipSequence()) {
            auto track = clip->clipSequence()->track();
            auto trackSequenceViewModel = singingClipPerTrackSequenceViewModelMap.value(track);
            if (trackSequenceViewModel) {
                trackSequenceViewModel->removeItem(q_ptr->getClipViewItemFromDocumentItem(clip));
            }
        }

        auto noteSequenceViewModel = noteSequenceViewModelMap.take(clip);
        singingClipFromNoteSequenceViewModelMap.remove(noteSequenceViewModel);
        if (noteSequenceViewModel) {
            noteSequenceViewModel->deleteLater();
        }
    }

    void NoteViewModelContextData::bindNoteDocumentItem(dspx::Note *item) {
        if (noteViewItemMap.contains(item)) {
            return;
        }

        auto clip = item->noteSequence()->singingClip();
        auto noteSequenceViewModel = noteSequenceViewModelMap.value(clip);
        Q_ASSERT(noteSequenceViewModel);

        auto viewItem = new sflow::NoteViewModel(noteSequenceViewModel);
        noteViewItemMap.insert(item, viewItem);
        noteDocumentItemMap.insert(viewItem, item);

        connect(item, &dspx::Note::lyricChanged, viewItem, [=] {
            if (viewItem->lyric() == item->lyric()) {
                return;
            }
            qCDebug(lcNoteViewModelContextData) << "Note lyric updated" << item << item->lyric();
            viewItem->setLyric(item->lyric());
        });
        connect(item, &dspx::Note::posChanged, viewItem, [=] {
            if (viewItem->position() == item->pos()) {
                return;
            }
            qCDebug(lcNoteViewModelContextData) << "Note position updated" << item << item->pos();
            viewItem->setPosition(item->pos());
        });
        connect(item, &dspx::Note::lengthChanged, viewItem, [=] {
            if (viewItem->length() == item->length()) {
                return;
            }
            qCDebug(lcNoteViewModelContextData) << "Note length updated" << item << item->length();
            viewItem->setLength(item->length());
        });
        connect(item, &dspx::Note::keyNumChanged, viewItem, [=] {
            if (viewItem->key() == item->keyNum()) {
                return;
            }
            qCDebug(lcNoteViewModelContextData) << "Note key updated" << item << item->keyNum();
            viewItem->setKey(item->keyNum());
        });
        connect(item, &dspx::Note::overlappedChanged, viewItem, [=](bool overlapped) {
            if (viewItem->isOverlapped() == overlapped) {
                return;
            }
            qCDebug(lcNoteViewModelContextData) << "Note overlapped updated" << item << overlapped;
            viewItem->setOverlapped(overlapped);
        });
        auto pronunciation = item->pronunciation();
        connect(pronunciation, &dspx::Pronunciation::originalChanged, viewItem, [=] {
            const auto additional = pronunciationAdditionalText(pronunciation);
            if (viewItem->additionalText() != additional) {
                viewItem->setAdditionalText(additional);
            }
            const bool highlighted = !pronunciation->edited().isEmpty();
            if (viewItem->isAdditionalTextHighlighted() != highlighted) {
                viewItem->setAdditionalTextHighlighted(highlighted);
            }
        });
        connect(pronunciation, &dspx::Pronunciation::editedChanged, viewItem, [=] {
            const auto additional = pronunciationAdditionalText(pronunciation);
            if (viewItem->additionalText() != additional) {
                viewItem->setAdditionalText(additional);
            }
            const bool highlighted = !pronunciation->edited().isEmpty();
            if (viewItem->isAdditionalTextHighlighted() != highlighted) {
                viewItem->setAdditionalTextHighlighted(highlighted);
            }
        });

        connect(viewItem, &sflow::NoteViewModel::positionChanged, item, [=, this] {
            if (!(stateMachine->configuration().contains(moveProcessingState) || stateMachine->configuration().contains(adjustProcessingState) || stateMachine->configuration().contains(drawProcessingState))) {
                viewItem->setPosition(item->pos());
                return;
            }
            qCDebug(lcNoteViewModelContextData) << "Note view position updated" << viewItem << viewItem->position();

            auto duplicateSelectionIfNeeded = [=, this] {
                if (!shouldCopyBeforeMove || !stateMachine->configuration().contains(moveProcessingState)) {
                    return;
                }
                shouldCopyBeforeMove = false;
                for (auto selectedItem : noteSelectionModel->selectedItems()) {
                    auto duplicated = duplicateNote(selectedItem, document->model());
                    if (!duplicated) {
                        continue;
                    }
                    if (auto sequence = selectedItem->noteSequence()) {
                        sequence->insertItem(duplicated);
                    }
                }
            };
            duplicateSelectionIfNeeded();
            item->setPos(viewItem->position());
            if (stateMachine->configuration().contains(moveProcessingState)) {
                moveChanged = true;
                moveUpdatedNotes.insert(viewItem);
            }
            if (stateMachine->configuration().contains(adjustProcessingState)) {
                lengthChanged = true;
                lengthUpdatedNotes.insert(viewItem);
            }
            if (stateMachine->configuration().contains(drawProcessingState)) {
                drawChanged = true;
                targetNote = item;
            }
        });
        connect(viewItem, &sflow::NoteViewModel::lengthChanged, item, [=, this] {
            if (!(stateMachine->configuration().contains(adjustProcessingState) || stateMachine->configuration().contains(drawProcessingState))) {
                viewItem->setLength(item->length());
                return;
            }
            qCDebug(lcNoteViewModelContextData) << "Note view length updated" << viewItem << viewItem->length();
            item->setLength(viewItem->length());
            if (stateMachine->configuration().contains(adjustProcessingState)) {
                lengthChanged = true;
                lengthUpdatedNotes.insert(viewItem);
            }
            if (stateMachine->configuration().contains(drawProcessingState)) {
                drawChanged = true;
                targetNote = item;
            }
        });
        connect(viewItem, &sflow::NoteViewModel::keyChanged, item, [=, this] {
            if (!(stateMachine->configuration().contains(moveProcessingState) || stateMachine->configuration().contains(drawProcessingState))) {
                viewItem->setKey(item->keyNum());
                return;
            }
            qCDebug(lcNoteViewModelContextData) << "Note view key updated" << viewItem << viewItem->key();

            if (shouldCopyBeforeMove && stateMachine->configuration().contains(moveProcessingState)) {
                shouldCopyBeforeMove = false;
                for (auto selectedItem : noteSelectionModel->selectedItems()) {
                    auto duplicated = duplicateNote(selectedItem, document->model());
                    if (!duplicated) {
                        continue;
                    }
                    if (auto sequence = selectedItem->noteSequence()) {
                        sequence->insertItem(duplicated);
                    }
                }
            }
            item->setKeyNum(viewItem->key());
            if (stateMachine->configuration().contains(moveProcessingState)) {
                moveChanged = true;
                moveUpdatedNotes.insert(viewItem);
            }
            if (stateMachine->configuration().contains(drawProcessingState)) {
                drawChanged = true;
                targetNote = item;
            }
        });
        connect(viewItem, &sflow::NoteViewModel::lyricChanged, item, [=, this] {
            if (!stateMachine->configuration().contains(lyricProgressingState)) {
                viewItem->setLyric(item->lyric());
                return;
            }
            lyricChanged = true;
            lyricUpdatedNotes.insert(viewItem);
        });
        connect(viewItem, &sflow::NoteViewModel::additionalTextChanged, item, [=, this] {
            if (!stateMachine->configuration().contains(additionalTextProgressingState)) {
                const auto additional = pronunciationAdditionalText(item->pronunciation());
                viewItem->setAdditionalText(additional);
                viewItem->setAdditionalTextHighlighted(!item->pronunciation()->edited().isEmpty());
                return;
            }
            additionalTextChanged = true;
            additionalTextUpdatedNotes.insert(viewItem);
        });

        viewItem->setLyric(item->lyric());
        viewItem->setPosition(item->pos());
        viewItem->setLength(item->length());
        viewItem->setKey(item->keyNum());
        viewItem->setOverlapped(item->isOverlapped());
        const auto additional = pronunciationAdditionalText(item->pronunciation());
        viewItem->setAdditionalText(additional);
        viewItem->setAdditionalTextHighlighted(!item->pronunciation()->edited().isEmpty());

        noteSequenceViewModel->insertItem(viewItem);
    }

    void NoteViewModelContextData::unbindNoteDocumentItem(dspx::Note *item) {
        if (!noteViewItemMap.contains(item)) {
            return;
        }
        auto viewItem = noteViewItemMap.take(item);
        noteDocumentItemMap.remove(viewItem);

        disconnect(item, nullptr, viewItem, nullptr);
        disconnect(viewItem, nullptr, item, nullptr);

        moveUpdatedNotes.remove(viewItem);
        lengthUpdatedNotes.remove(viewItem);
        lyricUpdatedNotes.remove(viewItem);
        additionalTextUpdatedNotes.remove(viewItem);

        if (item->noteSequence()) {
            auto clip = item->noteSequence()->singingClip();
            auto sequenceViewModel = noteSequenceViewModelMap.value(clip);
            if (sequenceViewModel) {
                sequenceViewModel->removeItem(viewItem);
            }
        }

        viewItem->deleteLater();
    }

    sflow::NoteEditLayerInteractionController *NoteViewModelContextData::createController(QObject *parent) {
        auto controller = new NoteEditLayerInteractionControllerProxy(this, parent);
        controller->setPrimaryItemInteraction(sflow::NoteEditLayerInteractionController::Move);
        controller->setSecondaryItemInteraction(sflow::NoteEditLayerInteractionController::CopyAndMove);
        controller->setPrimarySceneInteraction(sflow::NoteEditLayerInteractionController::RubberBandSelect);
        controller->setSecondarySceneInteraction(sflow::NoteEditLayerInteractionController::TimeRangeSelect);
        controller->setPrimarySelectInteraction(sflow::NoteEditLayerInteractionController::RubberBandSelect);
        controller->setSecondarySelectInteraction(sflow::NoteEditLayerInteractionController::TimeRangeSelect);

        connect(controller, &sflow::NoteEditLayerInteractionController::rubberBandDraggingStarted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillStart();
        });
        connect(controller, &sflow::NoteEditLayerInteractionController::rubberBandDraggingCommitted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillFinish();
        });
        connect(controller, &sflow::NoteEditLayerInteractionController::rubberBandDraggingAborted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillFinish();
        });

        connect(controller, &sflow::NoteEditLayerInteractionController::movingStarted, this, [=](QQuickItem *, sflow::NoteViewModel *, sflow::NoteEditLayerInteractionController::MoveFlag flag) {
            shouldCopyBeforeMove = (flag == sflow::NoteEditLayerInteractionController::MF_CopyAndMove);
            moveChanged = false;
            Q_EMIT moveTransactionWillStart();
        });
        connect(controller, &sflow::NoteEditLayerInteractionController::movingCommitted, this, [=](QQuickItem *, sflow::NoteViewModel *) {
            Q_EMIT moveTransactionWillCommit();
        });
        connect(controller, &sflow::NoteEditLayerInteractionController::movingAborted, this, [=](QQuickItem *, sflow::NoteViewModel *) {
            Q_EMIT moveTransactionWillAbort();
        });

        connect(controller, &sflow::NoteEditLayerInteractionController::adjustLengthStarted, this, [=](QQuickItem *, sflow::NoteViewModel *viewItem, sflow::NoteEditLayerInteractionController::AdjustLengthEdge) {
            targetNote = noteDocumentItemMap.value(viewItem);
            lengthChanged = false;
            Q_EMIT adjustTransactionWillStart();
        });
        connect(controller, &sflow::NoteEditLayerInteractionController::adjustLengthCommitted, this, [=](QQuickItem *, sflow::NoteViewModel *) {
            Q_EMIT adjustTransactionWillCommit();
        });
        connect(controller, &sflow::NoteEditLayerInteractionController::adjustLengthAborted, this, [=](QQuickItem *, sflow::NoteViewModel *) {
            Q_EMIT adjustTransactionWillAbort();
        });

        connect(controller, &sflow::NoteEditLayerInteractionController::drawingStarted, this, [=](QQuickItem *noteArea) {
            targetNoteArea = noteArea;
        });
        connect(controller, &sflow::NoteEditLayerInteractionController::drawingCommitted, this, [=](QQuickItem *) {
            Q_EMIT drawTransactionWillCommit();
        });
        connect(controller, &sflow::NoteEditLayerInteractionController::drawingAborted, this, [=](QQuickItem *) {
            Q_EMIT drawTransactionWillAbort();
        });

        connect(controller, &sflow::NoteEditLayerInteractionController::splitAboutToStart, this, [=](QQuickItem *) {
            splitPosition = {};
            Q_EMIT splitWillStart();
        });
        connect(controller, &sflow::NoteEditLayerInteractionController::splitCommitted, this, [=](QQuickItem *, int position) {
            splitPosition = position;
            Q_EMIT splitWillCommit();
        });
        connect(controller, &sflow::NoteEditLayerInteractionController::splitAborted, this, [=](QQuickItem *) {
            splitPosition = {};
            Q_EMIT splitWillAbort();
        });

        connect(controller, &sflow::NoteEditLayerInteractionController::itemDoubleClicked, this, [=](QQuickItem *noteArea, sflow::NoteViewModel *viewItem) {
            targetNoteArea = noteArea;
            targetNote = noteDocumentItemMap.value(viewItem);
            lyricChanged = false;
            Q_EMIT lyricTransactionWillStart();
        });

        connect(controller, &sflow::NoteEditLayerInteractionController::itemAdditionalTextDoubleClicked, this, [=](QQuickItem *noteArea, sflow::NoteViewModel *viewItem) {
            targetNoteArea = noteArea;
            targetNote = noteDocumentItemMap.value(viewItem);
            additionalTextChanged = false;
            Q_EMIT additionalTextTransactionWillStart();
        });

        connect(controller, &sflow::NoteEditLayerInteractionController::lyricInPlaceEditOperationTriggered, this, [=](QQuickItem *noteArea, sflow::NoteViewModel *viewItem, sflow::NoteEditLayerInteractionController::InPlaceEditOperation operation) {
            targetNoteArea = noteArea;
            targetNote = noteDocumentItemMap.value(viewItem);
            switch (operation) {
                case sflow::NoteEditLayerInteractionController::StartEditing:
                    lyricChanged = false;
                    Q_EMIT lyricTransactionWillStart();
                    break;
                case sflow::NoteEditLayerInteractionController::CommitEditing:
                    Q_EMIT lyricTransactionWillCommit();
                    break;
                case sflow::NoteEditLayerInteractionController::AbortEditing:
                    Q_EMIT lyricTransactionWillAbort();
                    break;
                case sflow::NoteEditLayerInteractionController::MovePrevious:
                case sflow::NoteEditLayerInteractionController::MoveNext:
                case sflow::NoteEditLayerInteractionController::MoveHome:
                case sflow::NoteEditLayerInteractionController::MoveEnd: {
                    auto sequence = targetNote->noteSequence();
                    dspx::Note *newTarget = nullptr;
                    if (operation == sflow::NoteEditLayerInteractionController::MovePrevious) {
                        newTarget = sequence->previousItem(targetNote);
                    } else if (operation == sflow::NoteEditLayerInteractionController::MoveNext) {
                        newTarget = sequence->nextItem(targetNote);
                    } else if (operation == sflow::NoteEditLayerInteractionController::MoveHome) {
                        newTarget = sequence->firstItem();
                    } else {
                        newTarget = sequence->lastItem();
                    }
                    if (newTarget) {
                        targetNote = newTarget;
                        auto newViewItem = noteViewItemMap.value(targetNote);
                        QMetaObject::invokeMethod(targetNoteArea, "editLyricInPlace", Q_ARG(sflow::NoteViewModel *, newViewItem));
                    }
                    break;
                }
            }
        });

        connect(controller, &sflow::NoteEditLayerInteractionController::additionalTextInPlaceEditOperationTriggered, this, [=](QQuickItem *noteArea, sflow::NoteViewModel *viewItem, sflow::NoteEditLayerInteractionController::InPlaceEditOperation operation) {
            targetNoteArea = noteArea;
            targetNote = noteDocumentItemMap.value(viewItem);
            switch (operation) {
                case sflow::NoteEditLayerInteractionController::StartEditing:
                    additionalTextChanged = false;
                    Q_EMIT additionalTextTransactionWillStart();
                    break;
                case sflow::NoteEditLayerInteractionController::CommitEditing:
                    Q_EMIT additionalTextTransactionWillCommit();
                    break;
                case sflow::NoteEditLayerInteractionController::AbortEditing:
                    Q_EMIT additionalTextTransactionWillAbort();
                    break;
                case sflow::NoteEditLayerInteractionController::MovePrevious:
                case sflow::NoteEditLayerInteractionController::MoveNext:
                case sflow::NoteEditLayerInteractionController::MoveHome:
                case sflow::NoteEditLayerInteractionController::MoveEnd: {
                    auto sequence = targetNote->noteSequence();
                    dspx::Note *newTarget = nullptr;
                    if (operation == sflow::NoteEditLayerInteractionController::MovePrevious) {
                        newTarget = sequence->previousItem(targetNote);
                    } else if (operation == sflow::NoteEditLayerInteractionController::MoveNext) {
                        newTarget = sequence->nextItem(targetNote);
                    } else if (operation == sflow::NoteEditLayerInteractionController::MoveHome) {
                        newTarget = sequence->firstItem();
                    } else {
                        newTarget = sequence->lastItem();
                    }
                    if (newTarget) {
                        targetNote = newTarget;
                        auto newViewItem = noteViewItemMap.value(targetNote);
                        QMetaObject::invokeMethod(targetNoteArea, "editAdditionalTextInPlace", Q_ARG(sflow::NoteViewModel *, newViewItem));
                    }
                    break;
                }
            }
        });

        return controller;
    }

    void NoteViewModelContextData::onMovePendingStateEntered() {
        moveTransactionId = document->transactionController()->beginTransaction();
        moveUpdatedNotes.clear();
        if (moveTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT moveTransactionStarted();
        } else {
            Q_EMIT moveTransactionNotStarted();
        }
    }

    void NoteViewModelContextData::onMoveCommittingStateEntered() {
        if (!moveChanged) {
            document->transactionController()->abortTransaction(moveTransactionId);
        } else {
            document->transactionController()->commitTransaction(moveTransactionId, tr("Moving note"));
        }
        moveTransactionId = {};
        moveChanged = false;
        moveUpdatedNotes.clear();
        shouldCopyBeforeMove = false;
    }

    void NoteViewModelContextData::onMoveAbortingStateEntered() {
        document->transactionController()->abortTransaction(moveTransactionId);
        moveTransactionId = {};
        moveChanged = false;
        moveUpdatedNotes.clear();
        shouldCopyBeforeMove = false;
    }

    void NoteViewModelContextData::onAdjustPendingStateEntered() {
        adjustTransactionId = document->transactionController()->beginTransaction();
        lengthChanged = false;
        lengthUpdatedNotes.clear();
        if (adjustTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT adjustTransactionStarted();
        } else {
            Q_EMIT adjustTransactionNotStarted();
        }
    }

    void NoteViewModelContextData::onAdjustCommittingStateEntered() {
        if (!lengthChanged) {
            document->transactionController()->abortTransaction(adjustTransactionId);
        } else {
            for (auto viewItem : lengthUpdatedNotes) {
                auto documentItem = noteDocumentItemMap.value(viewItem);
                documentItem->setLength(viewItem->length());
                documentItem->setPos(viewItem->position());
                documentItem->setKeyNum(viewItem->key());
            }
            document->transactionController()->commitTransaction(adjustTransactionId, tr("Adjusting note length"));
        }
        adjustTransactionId = {};
        lengthChanged = false;
        lengthUpdatedNotes.clear();
        targetNote = {};
    }

    void NoteViewModelContextData::onAdjustAbortingStateEntered() {
        document->transactionController()->abortTransaction(adjustTransactionId);
        adjustTransactionId = {};
        lengthChanged = false;
        lengthUpdatedNotes.clear();
        targetNote = {};
    }

    void NoteViewModelContextData::onDrawPendingStateEntered() {
        drawTransactionId = document->transactionController()->beginTransaction();
        if (drawTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT drawTransactionStarted();
        } else {
            if (targetNote && targetNote->noteSequence()) {
                targetNote->noteSequence()->removeItem(targetNote);
            }
            targetNote = {};
            drawChanged = false;
            Q_EMIT drawTransactionNotStarted();
        }
    }

    void NoteViewModelContextData::onDrawCommittingStateEntered() {
        if (!targetNote || drawTransactionId == Core::TransactionController::TransactionId::Invalid) {
            targetNote = {};
            drawChanged = false;
            return;
        }

        auto viewItem = noteViewItemMap.value(targetNote);
        if (viewItem) {
            targetNote->setPos(viewItem->position());
            targetNote->setLength(viewItem->length());
            targetNote->setKeyNum(viewItem->key());
        }

        if (!drawChanged) {
            document->transactionController()->abortTransaction(drawTransactionId);
        } else {
            document->transactionController()->commitTransaction(drawTransactionId, tr("Drawing note"));
            document->selectionModel()->select(targetNote, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection, dspx::SelectionModel::ST_Note);
        }

        drawTransactionId = {};
        drawChanged = false;
        targetNote = {};
        targetNoteArea = {};
    }

    void NoteViewModelContextData::onDrawAbortingStateEntered() {
        document->transactionController()->abortTransaction(drawTransactionId);
        drawTransactionId = {};
        drawChanged = false;
        targetNote = {};
        targetNoteArea = {};
    }

    void NoteViewModelContextData::onSplitCommittingStateEntered() {
        document->splitItems(splitPosition);
        splitPosition = {};
    }

    void NoteViewModelContextData::onLyricPendingStateEntered() {
        lyricTransactionId = document->transactionController()->beginTransaction();
        if (lyricTransactionId != Core::TransactionController::TransactionId::Invalid) {
            auto viewItem = noteViewItemMap.value(targetNote);
            if (targetNoteArea && viewItem) {
                QMetaObject::invokeMethod(targetNoteArea, "editLyricInPlace", Q_ARG(sflow::NoteViewModel *, viewItem));
            }
            Q_EMIT lyricTransactionStarted();
        } else {
            Q_EMIT lyricTransactionNotStarted();
        }
    }

    void NoteViewModelContextData::onLyricCommittingStateEntered() {
        auto viewItem = noteViewItemMap.value(targetNote);
        if (viewItem && targetNote->lyric() != viewItem->lyric()) {
            targetNote->setLyric(viewItem->lyric());
            document->transactionController()->commitTransaction(lyricTransactionId, tr("Editing lyric"));
        } else {
            document->transactionController()->abortTransaction(lyricTransactionId);
        }
        lyricTransactionId = {};
        lyricChanged = false;
        lyricUpdatedNotes.clear();
        targetNote = {};
        targetNoteArea = {};
    }

    void NoteViewModelContextData::onLyricAbortingStateEntered() {
        document->transactionController()->abortTransaction(lyricTransactionId);
        lyricTransactionId = {};
        lyricChanged = false;
        lyricUpdatedNotes.clear();
        targetNote = {};
        targetNoteArea = {};
    }

    void NoteViewModelContextData::onAdditionalTextPendingStateEntered() {
        additionalTextTransactionId = document->transactionController()->beginTransaction();
        if (additionalTextTransactionId != Core::TransactionController::TransactionId::Invalid) {
            auto viewItem = noteViewItemMap.value(targetNote);
            if (targetNoteArea && viewItem) {
                QMetaObject::invokeMethod(targetNoteArea, "editAdditionalTextInPlace", Q_ARG(sflow::NoteViewModel *, viewItem));
            }
            Q_EMIT additionalTextTransactionStarted();
        } else {
            Q_EMIT additionalTextTransactionNotStarted();
        }
    }

    void NoteViewModelContextData::onAdditionalTextCommittingStateEntered() {
        auto viewItem = noteViewItemMap.value(targetNote);
        auto pronunciation = targetNote ? targetNote->pronunciation() : nullptr;
        if (viewItem && pronunciation) {
            const auto previous = pronunciationAdditionalText(pronunciation);
            if (previous == viewItem->additionalText()) {
                document->transactionController()->abortTransaction(additionalTextTransactionId);
            } else {
                pronunciation->setEdited(viewItem->additionalText());
                document->transactionController()->commitTransaction(additionalTextTransactionId, tr("Editing pronunciation"));
            }
        } else {
            document->transactionController()->abortTransaction(additionalTextTransactionId);
        }
        additionalTextTransactionId = {};
        additionalTextChanged = false;
        additionalTextUpdatedNotes.clear();
        targetNote = {};
        targetNoteArea = {};
    }

    void NoteViewModelContextData::onAdditionalTextAbortingStateEntered() {
        document->transactionController()->abortTransaction(additionalTextTransactionId);
        additionalTextTransactionId = {};
        additionalTextChanged = false;
        additionalTextUpdatedNotes.clear();
        targetNote = {};
        targetNoteArea = {};
    }

    sflow::NoteViewModel *NoteEditLayerInteractionControllerProxy::createAndInsertNoteOnDrawing(sflow::RangeSequenceViewModel *noteSequenceViewModel, int position, int trackIndex) {
        if (!m_context) {
            return nullptr;
        }

        auto singingClip = m_context->singingClipFromNoteSequenceViewModelMap.value(noteSequenceViewModel);
        if (!singingClip) {
            return nullptr;
        }

        Q_EMIT m_context->drawTransactionWillStart();

        auto note = m_context->document->model()->createNote();
        note->setPos(position);
        note->setLength(0);
        note->setKeyNum(trackIndex);
        singingClip->notes()->insertItem(note);

        m_context->targetNote = note;
        m_context->drawChanged = true;

        auto viewItem = m_context->noteViewItemMap.value(note);
        if (viewItem) {
            viewItem->setPosition(position);
            viewItem->setLength(0);
            viewItem->setKey(trackIndex);
        }

        Q_UNUSED(noteSequenceViewModel)
        return viewItem;
    }

}