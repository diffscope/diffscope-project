#include "ClipViewModelContextData_p.h"

#include <limits>

#include <QLoggingCategory>
#include <QQuickItem>
#include <QState>
#include <QStateMachine>
#include <QUrl>

#include <ScopicFlowCore/ClipPaneInteractionController.h>
#include <ScopicFlowCore/ClipViewModel.h>
#include <ScopicFlowCore/RangeSequenceViewModel.h>

#include <dspxmodel/AudioClip.h>
#include <dspxmodel/BusControl.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipSelectionModel.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/Workspace.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/private/ClipSelectionController_p.h>

namespace VisualEditor {

    namespace {
        QUrl clipIconForType(dspx::Clip::ClipType type) {
            if (type == dspx::Clip::Singing) {
                return QUrl("image://fluent-system-icons/mic");
            }
            return QUrl("image://fluent-system-icons/sound_wave_circle");
        }

        int viewMaxLengthFromDocument(int length) {
            return length == 0 ? std::numeric_limits<int>::max() : length;
        }

        dspx::Clip *duplicateClip(dspx::Clip *source, Core::DspxDocument *document) {
            if (!source) {
                return nullptr;
            }
            dspx::Clip *result = nullptr;
            if (source->type() == dspx::Clip::Audio) {
                result = document->model()->createAudioClip();
            } else {
                result = document->model()->createSingingClip();
            }
            result->fromQDspx(source->toQDspx());
            return result;
        }

        class ClipPaneInteractionControllerProxy : public sflow::ClipPaneInteractionController {
        public:
            ClipPaneInteractionControllerProxy(ClipViewModelContextData *context, QObject *parent)
                : sflow::ClipPaneInteractionController(parent), m_context(context) {
            }

            sflow::ClipViewModel *createAndInsertClipOnDrawing(sflow::RangeSequenceViewModel *clipSequenceViewModel, int position, int trackIndex) override;

        private:
            ClipViewModelContextData *m_context;
        };
    }

    Q_STATIC_LOGGING_CATEGORY(lcClipViewModelContextData, "diffscope.visualeditor.clipviewmodelcontextdata")

    void ClipViewModelContextData::initStateMachine() {
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
        stateMachine->setInitialState(idleState);
        stateMachine->start();

        idleState->addTransition(this, &ClipViewModelContextData::rubberBandDragWillStart, rubberBandDraggingState);
        rubberBandDraggingState->addTransition(this, &ClipViewModelContextData::rubberBandDragWillFinish, idleState);

        idleState->addTransition(this, &ClipViewModelContextData::moveTransactionWillStart, movePendingState);
        movePendingState->addTransition(this, &ClipViewModelContextData::moveTransactionStarted, moveProcessingState);
        movePendingState->addTransition(this, &ClipViewModelContextData::moveTransactionNotStarted, idleState);
        moveProcessingState->addTransition(this, &ClipViewModelContextData::moveTransactionWillCommit, moveCommittingState);
        moveProcessingState->addTransition(this, &ClipViewModelContextData::moveTransactionWillAbort, moveAbortingState);
        moveCommittingState->addTransition(idleState);
        moveAbortingState->addTransition(idleState);

        idleState->addTransition(this, &ClipViewModelContextData::adjustTransactionWillStart, adjustPendingState);
        adjustPendingState->addTransition(this, &ClipViewModelContextData::adjustTransactionStarted, adjustProcessingState);
        adjustPendingState->addTransition(this, &ClipViewModelContextData::adjustTransactionNotStarted, idleState);
        adjustProcessingState->addTransition(this, &ClipViewModelContextData::adjustTransactionWillCommit, adjustCommittingState);
        adjustProcessingState->addTransition(this, &ClipViewModelContextData::adjustTransactionWillAbort, adjustAbortingState);
        adjustCommittingState->addTransition(idleState);
        adjustAbortingState->addTransition(idleState);

        idleState->addTransition(this, &ClipViewModelContextData::drawTransactionWillStart, drawPendingState);
        drawPendingState->addTransition(this, &ClipViewModelContextData::drawTransactionStarted, drawProcessingState);
        drawPendingState->addTransition(this, &ClipViewModelContextData::drawTransactionNotStarted, idleState);
        drawProcessingState->addTransition(this, &ClipViewModelContextData::drawTransactionWillCommit, drawCommittingState);
        drawProcessingState->addTransition(this, &ClipViewModelContextData::drawTransactionWillAbort, drawAbortingState);
        drawCommittingState->addTransition(idleState);
        drawAbortingState->addTransition(idleState);

        auto logEntered = [](const char *name) { qCInfo(lcClipViewModelContextData) << name << "entered"; };
        auto logExited = [](const char *name) { qCInfo(lcClipViewModelContextData) << name << "exited"; };

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
    }

    void ClipViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        document = q->windowHandle()->projectDocumentContext()->document();
        trackList = document->model()->tracks();
        selectionModel = document->selectionModel();
        clipSelectionModel = selectionModel->clipSelectionModel();

        clipSequenceViewModel = new sflow::RangeSequenceViewModel(q, "position", "length");
        clipSelectionController = new ClipSelectionController(q);

        initStateMachine();
    }

    void ClipViewModelContextData::bindClipSequences() {
        connect(trackList, &dspx::TrackList::itemInserted, this, [=, this](int, dspx::Track *track) {
            bindTrack(track);
            if (!stateMachine->configuration().contains(moveProcessingState)) {
                for (auto documentItem : clipViewItemMap.keys()) {
                    auto viewItem = clipViewItemMap.value(documentItem);
                    const auto trackIndex = trackList->items().indexOf(documentItem->clipSequence()->track());
                    if (trackIndex >= 0 && viewItem->trackIndex() != trackIndex) {
                        viewItem->setTrackIndex(trackIndex);
                    }
                }
            }
        });
        connect(trackList, &dspx::TrackList::itemRemoved, this, [=, this](int, dspx::Track *track) {
            unbindTrack(track);
            if (!stateMachine->configuration().contains(moveProcessingState)) {
                for (auto documentItem : clipViewItemMap.keys()) {
                    auto viewItem = clipViewItemMap.value(documentItem);
                    const auto trackIndex = trackList->items().indexOf(documentItem->clipSequence()->track());
                    if (trackIndex >= 0 && viewItem->trackIndex() != trackIndex) {
                        viewItem->setTrackIndex(trackIndex);
                    }
                }
            }
        });
        connect(trackList, &dspx::TrackList::rotated, this, [=, this](int, int, int) {
            if (stateMachine->configuration().contains(moveProcessingState)) {
                return;
            }
            for (auto documentItem : clipViewItemMap.keys()) {
                auto viewItem = clipViewItemMap.value(documentItem);
                const auto trackIndex = trackList->items().indexOf(documentItem->clipSequence()->track());
                if (trackIndex >= 0 && viewItem->trackIndex() != trackIndex) {
                    viewItem->setTrackIndex(trackIndex);
                }
            }
        });

        for (auto track : trackList->items()) {
            bindTrack(track);
        }

        connect(clipSelectionModel, &dspx::ClipSelectionModel::itemSelected, this, [=, this](dspx::Clip *item, bool selected) {
            auto viewItem = clipViewItemMap.value(item);
            Q_ASSERT(viewItem);
            viewItem->setSelected(selected);
        });

        for (auto selectedItem : clipSelectionModel->selectedItems()) {
            auto viewItem = clipViewItemMap.value(selectedItem);
            if (!viewItem) {
                continue;
            }
            viewItem->setSelected(true);
        }
    }

    void ClipViewModelContextData::bindTrack(dspx::Track *track) {
        if (!track) {
            return;
        }
        auto sequence = track->clips();
        connect(sequence, &dspx::ClipSequence::itemInserted, this, [=, this](dspx::Clip *item) {
            bindClipDocumentItem(item);
        });
        connect(sequence, &dspx::ClipSequence::itemRemoved, this, [=, this](dspx::Clip *item) {
            unbindClipDocumentItem(item);
        });

        for (auto item : sequence->asRange()) {
            bindClipDocumentItem(item);
        }
    }

    void ClipViewModelContextData::unbindTrack(dspx::Track *track) {
        if (!track) {
            return;
        }
        auto sequence = track->clips();
        disconnect(sequence, nullptr, this, nullptr);
        const auto items = clipViewItemMap.keys();
        for (auto documentItem : items) {
            if (documentItem->clipSequence() && documentItem->clipSequence()->track() == track) {
                unbindClipDocumentItem(documentItem);
            }
        }
    }

    void ClipViewModelContextData::bindClipDocumentItem(dspx::Clip *item) {
        if (!item || clipViewItemMap.contains(item)) {
            return;
        }
        auto viewItem = new sflow::ClipViewModel(clipSequenceViewModel);
        viewItem->setIconSource(clipIconForType(item->type()));
        clipViewItemMap.insert(item, viewItem);
        clipDocumentItemMap.insert(viewItem, item);

        auto time = item->time();
        auto control = item->control();

        connect(item, &dspx::Clip::nameChanged, viewItem, [=] {
            if (viewItem->name() == item->name()) {
                return;
            }
            viewItem->setName(item->name());
        });
        connect(item, &dspx::Clip::positionChanged, viewItem, [=] {
            if (viewItem->position() == item->position()) {
                return;
            }
            viewItem->setPosition(item->position());
        });
        connect(item, &dspx::Clip::lengthChanged, viewItem, [=] {
            if (viewItem->length() == time->clipLen()) {
                return;
            }
            viewItem->setLength(time->clipLen());
        });
        connect(item, &dspx::Clip::overlappedChanged, viewItem, [=](bool overlapped) {
            if (viewItem->isOverlapped() == overlapped) {
                return;
            }
            viewItem->setOverlapped(overlapped);
        });
        connect(item, &dspx::Clip::clipSequenceChanged, viewItem, [=, this] {
            if (stateMachine->configuration().contains(moveProcessingState)) {
                return;
            }
            if (!item->clipSequence()) {
                return;
            }
            const auto trackIndex = trackList->items().indexOf(item->clipSequence()->track());
            if (trackIndex >= 0 && viewItem->trackIndex() != trackIndex) {
                viewItem->setTrackIndex(trackIndex);
            }
        });
        connect(time, &dspx::ClipTime::clipStartChanged, viewItem, [=, this](int clipStart) {
            const int documentPosition = item->position();
            if (viewItem->clipStart() != clipStart) {
                viewItem->setClipStart(clipStart);
            }
            if (viewItem->position() != documentPosition) {
                viewItem->setPosition(documentPosition);
            }
        });
        connect(time, &dspx::ClipTime::clipLenChanged, viewItem, [=](int clipLen) {
            if (viewItem->length() == clipLen) {
                return;
            }
            viewItem->setLength(clipLen);
        });
        connect(time, &dspx::ClipTime::lengthChanged, viewItem, [=](int length) {
            const int maxLength = viewMaxLengthFromDocument(length);
            if (viewItem->maxLength() == maxLength) {
                return;
            }
            viewItem->setMaxLength(maxLength);
        });
        connect(control, &dspx::Control::muteChanged, viewItem, [=](bool mute) {
            if (viewItem->isMute() == mute) {
                return;
            }
            viewItem->setMute(mute);
        });

        connect(viewItem, &sflow::ClipViewModel::positionChanged, item, [=, this] {
            if (!(stateMachine->configuration().contains(moveProcessingState) || stateMachine->configuration().contains(adjustProcessingState) || stateMachine->configuration().contains(drawProcessingState))) {
                viewItem->setPosition(item->position());
                return;
            }

            auto duplicateSelectionIfNeeded = [=, this] {
                if (!shouldCopyBeforeMove || !stateMachine->configuration().contains(moveProcessingState)) {
                    return;
                }
                for (auto selectedItem : clipSelectionModel->selectedItems()) {
                    auto duplicated = duplicateClip(selectedItem, document);
                    if (!duplicated) {
                        continue;
                    }
                    selectedItem->clipSequence()->insertItem(duplicated);
                }
                shouldCopyBeforeMove = false;
            };
            duplicateSelectionIfNeeded();

            const int newStart = viewItem->position() - viewItem->clipStart();
            time->setStart(newStart);
            if (stateMachine->configuration().contains(moveProcessingState)) {
                moveChanged = true;
                moveUpdatedClips.insert(viewItem);
            }
            if (stateMachine->configuration().contains(adjustProcessingState)) {
                lengthChanged = true;
                lengthUpdatedClips.insert(viewItem);
            }
            if (stateMachine->configuration().contains(drawProcessingState)) {
                drawChanged = true;
                targetClip = item;
            }
        });
        connect(viewItem, &sflow::ClipViewModel::lengthChanged, item, [=, this] {
            if (!(stateMachine->configuration().contains(adjustProcessingState) || stateMachine->configuration().contains(drawProcessingState))) {
                viewItem->setLength(time->clipLen());
                return;
            }
            time->setClipLen(viewItem->length());
            if (stateMachine->configuration().contains(adjustProcessingState)) {
                lengthChanged = true;
                lengthUpdatedClips.insert(viewItem);
            }
            if (stateMachine->configuration().contains(drawProcessingState)) {
                drawChanged = true;
                targetClip = item;
            }
        });
        connect(viewItem, &sflow::ClipViewModel::clipStartChanged, item, [=, this] {
            if (!(stateMachine->configuration().contains(adjustProcessingState) || stateMachine->configuration().contains(drawProcessingState))) {
                viewItem->setClipStart(time->clipStart());
                viewItem->setPosition(item->position());
                return;
            }
            time->setClipStart(viewItem->clipStart());
            const int newStart = viewItem->position() - viewItem->clipStart();
            time->setStart(newStart);
            if (stateMachine->configuration().contains(adjustProcessingState)) {
                lengthChanged = true;
                lengthUpdatedClips.insert(viewItem);
            }
            if (stateMachine->configuration().contains(drawProcessingState)) {
                drawChanged = true;
                targetClip = item;
            }
        });
        connect(viewItem, &sflow::ClipViewModel::trackIndexChanged, item, [=, this] {
            // FIXME temporarily bypass this, sync model fix later
            const int currentIndex = trackList->items().indexOf(item->clipSequence()->track());
            if (currentIndex >= 0 && viewItem->trackIndex() != currentIndex) {
                viewItem->setTrackIndex(currentIndex);
            }
            return;
            if (!stateMachine->configuration().contains(moveProcessingState)) {
                const int currentIndex = trackList->items().indexOf(item->clipSequence()->track());
                if (currentIndex >= 0 && viewItem->trackIndex() != currentIndex) {
                    viewItem->setTrackIndex(currentIndex);
                }
                return;
            }
            if (shouldCopyBeforeMove) {
                for (auto selectedItem : clipSelectionModel->selectedItems()) {
                    auto duplicated = duplicateClip(selectedItem, document);
                    if (!duplicated) {
                        continue;
                    }
                    selectedItem->clipSequence()->insertItem(duplicated);
                }
                shouldCopyBeforeMove = false;
            }
            const int newIndex = viewItem->trackIndex();
            const auto tracks = trackList->items();
            if (newIndex < 0 || newIndex >= tracks.size()) {
                const int currentIndex = tracks.indexOf(item->clipSequence()->track());
                if (currentIndex >= 0) {
                    viewItem->setTrackIndex(currentIndex);
                }
                return;
            }
            auto targetTrack = tracks.at(newIndex);
            if (targetTrack == item->clipSequence()->track()) {
                return;
            }
            moveChanged = true;
            moveUpdatedClips.insert(viewItem);
            item->clipSequence()->moveToAnotherClipSequence(item, targetTrack->clips());
        });

        viewItem->setName(item->name());
        viewItem->setPosition(item->position());
        viewItem->setClipStart(time->clipStart());
        viewItem->setLength(time->clipLen());
        viewItem->setMaxLength(viewMaxLengthFromDocument(time->length()));
        viewItem->setOverlapped(item->isOverlapped());
        viewItem->setMute(item->control()->mute());
        const int trackIndex = trackList->items().indexOf(item->clipSequence()->track());
        if (trackIndex >= 0) {
            viewItem->setTrackIndex(trackIndex);
        }

        clipSequenceViewModel->insertItem(viewItem);
    }

    void ClipViewModelContextData::unbindClipDocumentItem(dspx::Clip *item) {
        if (!clipViewItemMap.contains(item)) {
            return;
        }
        auto viewItem = clipViewItemMap.take(item);
        clipDocumentItemMap.remove(viewItem);

        disconnect(item, nullptr, viewItem, nullptr);
        disconnect(viewItem, nullptr, item, nullptr);

        moveUpdatedClips.remove(viewItem);
        lengthUpdatedClips.remove(viewItem);

        clipSequenceViewModel->removeItem(viewItem);

        viewItem->deleteLater();
    }

    sflow::ClipPaneInteractionController *ClipViewModelContextData::createController(QObject *parent) {
        auto controller = new ClipPaneInteractionControllerProxy(this, parent);
        controller->setPrimaryItemInteraction(sflow::ClipPaneInteractionController::Move);
        controller->setSecondaryItemInteraction(sflow::ClipPaneInteractionController::CopyAndMove);
        controller->setPrimarySceneInteraction(sflow::ClipPaneInteractionController::RubberBandSelect);
        controller->setSecondarySceneInteraction(sflow::ClipPaneInteractionController::TimeRangeSelect);
        controller->setPrimarySelectInteraction(sflow::ClipPaneInteractionController::RubberBandSelect);
        controller->setSecondarySelectInteraction(sflow::ClipPaneInteractionController::TimeRangeSelect);

        connect(controller, &sflow::ClipPaneInteractionController::rubberBandDraggingStarted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillStart();
        });
        connect(controller, &sflow::ClipPaneInteractionController::rubberBandDraggingCommitted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillFinish();
        });
        connect(controller, &sflow::ClipPaneInteractionController::rubberBandDraggingAborted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillFinish();
        });

        connect(controller, &sflow::ClipPaneInteractionController::movingStarted, this, [=](QQuickItem *, sflow::ClipViewModel *, sflow::ClipPaneInteractionController::MoveFlag moveFlag) {
            shouldCopyBeforeMove = (moveFlag == sflow::ClipPaneInteractionController::MF_CopyAndMove);
            moveChanged = false;
            Q_EMIT moveTransactionWillStart();
        });
        connect(controller, &sflow::ClipPaneInteractionController::movingCommitted, this, [=](QQuickItem *, sflow::ClipViewModel *) {
            Q_EMIT moveTransactionWillCommit();
        });
        connect(controller, &sflow::ClipPaneInteractionController::movingAborted, this, [=](QQuickItem *, sflow::ClipViewModel *) {
            Q_EMIT moveTransactionWillAbort();
        });

        connect(controller, &sflow::ClipPaneInteractionController::adjustLengthStarted, this, [=](QQuickItem *, sflow::ClipViewModel *viewItem) {
            targetClip = clipDocumentItemMap.value(viewItem);
            lengthChanged = false;
            Q_EMIT adjustTransactionWillStart();
        });
        connect(controller, &sflow::ClipPaneInteractionController::adjustLengthCommitted, this, [=](QQuickItem *, sflow::ClipViewModel *) {
            Q_EMIT adjustTransactionWillCommit();
        });
        connect(controller, &sflow::ClipPaneInteractionController::adjustLengthAborted, this, [=](QQuickItem *, sflow::ClipViewModel *) {
            Q_EMIT adjustTransactionWillAbort();
        });

        connect(controller, &sflow::ClipPaneInteractionController::drawingStarted, this, [=](QQuickItem *clipPane) {
            targetClipPane = clipPane;
        });
        connect(controller, &sflow::ClipPaneInteractionController::drawingCommitted, this, [=](QQuickItem *) {
            Q_EMIT drawTransactionWillCommit();
        });
        connect(controller, &sflow::ClipPaneInteractionController::drawingAborted, this, [=](QQuickItem *) {
            Q_EMIT drawTransactionWillAbort();
        });

        return controller;
    }

    void ClipViewModelContextData::onMovePendingStateEntered() {
        moveTransactionId = document->transactionController()->beginTransaction();
        moveUpdatedClips.clear();
        if (moveTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT moveTransactionStarted();
        } else {
            Q_EMIT moveTransactionNotStarted();
        }
    }

    void ClipViewModelContextData::onMoveCommittingStateEntered() {
        if (!moveChanged) {
            document->transactionController()->abortTransaction(moveTransactionId);
        } else {
            document->transactionController()->commitTransaction(moveTransactionId, tr("Moving clip"));
        }
        moveTransactionId = {};
        moveChanged = false;
        moveUpdatedClips.clear();
        shouldCopyBeforeMove = false;
    }

    void ClipViewModelContextData::onMoveAbortingStateEntered() {
        document->transactionController()->abortTransaction(moveTransactionId);
        moveTransactionId = {};
        moveChanged = false;
        moveUpdatedClips.clear();
        shouldCopyBeforeMove = false;
    }

    void ClipViewModelContextData::onAdjustPendingStateEntered() {
        adjustTransactionId = document->transactionController()->beginTransaction();
        lengthChanged = false;
        lengthUpdatedClips.clear();
        if (adjustTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT adjustTransactionStarted();
        } else {
            Q_EMIT adjustTransactionNotStarted();
        }
    }

    void ClipViewModelContextData::onAdjustCommittingStateEntered() {
        if (!lengthChanged) {
            document->transactionController()->abortTransaction(adjustTransactionId);
        } else {
            for (auto viewItem : lengthUpdatedClips) {
                auto documentItem = clipDocumentItemMap.value(viewItem);
                if (!documentItem) {
                    continue;
                }
                auto time = documentItem->time();
                time->setClipLen(viewItem->length());
                time->setClipStart(viewItem->clipStart());
                time->setStart(viewItem->position() - viewItem->clipStart());
            }
            document->transactionController()->commitTransaction(adjustTransactionId, tr("Adjusting clip length"));
        }
        adjustTransactionId = {};
        lengthChanged = false;
        lengthUpdatedClips.clear();
        targetClip = {};
    }

    void ClipViewModelContextData::onAdjustAbortingStateEntered() {
        document->transactionController()->abortTransaction(adjustTransactionId);
        adjustTransactionId = {};
        lengthChanged = false;
        lengthUpdatedClips.clear();
        targetClip = {};
    }

    void ClipViewModelContextData::onDrawPendingStateEntered() {
        drawTransactionId = document->transactionController()->beginTransaction();
        if (drawTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT drawTransactionStarted();
        } else {
            if (targetClip && targetClip->clipSequence()) {
                targetClip->clipSequence()->removeItem(targetClip);
            }
            targetClip = {};
            drawChanged = false;
            Q_EMIT drawTransactionNotStarted();
        }
    }

    void ClipViewModelContextData::onDrawCommittingStateEntered() {
        if (!targetClip || drawTransactionId == Core::TransactionController::TransactionId::Invalid) {
            targetClip = {};
            drawChanged = false;
            return;
        }

        auto viewItem = clipViewItemMap.value(targetClip);
        if (viewItem) {
            auto time = targetClip->time();
            time->setClipLen(viewItem->length());
            time->setClipStart(viewItem->clipStart());
            time->setStart(viewItem->position() - viewItem->clipStart());
        }

        if (!drawChanged) {
            document->transactionController()->abortTransaction(drawTransactionId);
        } else {
            document->transactionController()->commitTransaction(drawTransactionId, tr("Drawing clip"));
            document->selectionModel()->select(targetClip, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection, dspx::SelectionModel::ST_Clip);
        }

        drawTransactionId = {};
        drawChanged = false;
        targetClip = {};
        targetClipPane = {};
    }

    void ClipViewModelContextData::onDrawAbortingStateEntered() {
        document->transactionController()->abortTransaction(drawTransactionId);
        drawTransactionId = {};
        drawChanged = false;
        targetClip = {};
        targetClipPane = {};
    }

    sflow::ClipViewModel *ClipPaneInteractionControllerProxy::createAndInsertClipOnDrawing(sflow::RangeSequenceViewModel *clipSequenceViewModel, int position, int trackIndex) {
        if (!m_context || !m_context->trackList) {
            return nullptr;
        }

        const auto tracks = m_context->trackList->items();
        if (trackIndex < 0 || trackIndex >= tracks.size()) {
            return nullptr;
        }

        auto track = tracks.at(trackIndex);
        Q_EMIT m_context->drawTransactionWillStart();

        auto clip = m_context->document->model()->createSingingClip();
        clip->setName(tr("Unnamed clip"));
        auto time = clip->time();
        time->setClipStart(0);
        time->setClipLen(0);
        time->setStart(position);

        track->clips()->insertItem(clip);

        m_context->targetClip = clip;
        m_context->drawChanged = true;

        auto viewItem = m_context->clipViewItemMap.value(clip);
        if (viewItem) {
            viewItem->setTrackIndex(trackIndex);
            viewItem->setPosition(position);
            viewItem->setClipStart(time->clipStart());
            viewItem->setLength(time->clipLen());
        }

        Q_UNUSED(clipSequenceViewModel)
        return viewItem;
    }

}
