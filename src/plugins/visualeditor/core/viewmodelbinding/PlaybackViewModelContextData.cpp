#include "PlaybackViewModelContextData_p.h"

#include <algorithm>

#include <QLoggingCategory>
#include <QQuickItem>
#include <QState>
#include <QStateMachine>

#include <ScopicFlowCore/PlaybackViewModel.h>
#include <ScopicFlowCore/TimelineInteractionController.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/Timeline.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcPlaybackViewModelContextData, "diffscope.visualeditor.playbackviewmodelcontextdata")

    void PlaybackViewModelContextData::initStateMachine() {
        stateMachine = new QStateMachine(QState::ExclusiveStates, this);
        idleState = new QState;
        rubberBandDraggingState = new QState;
        positionIndicatorMovingState = new QState;
        loopRangeAdjustPendingState = new QState;
        loopRangeAdjustingState = new QState;
        stateMachine->addState(idleState);
        stateMachine->addState(rubberBandDraggingState);
        stateMachine->addState(positionIndicatorMovingState);
        stateMachine->addState(loopRangeAdjustPendingState);
        stateMachine->addState(loopRangeAdjustingState);
        stateMachine->setInitialState(idleState);
        stateMachine->start();

        idleState->addTransition(this, &PlaybackViewModelContextData::rubberBandDragWillStart, rubberBandDraggingState);
        rubberBandDraggingState->addTransition(this, &PlaybackViewModelContextData::rubberBandDragWillFinish, idleState);

        idleState->addTransition(this, &PlaybackViewModelContextData::positionIndicatorMoveWillStart, positionIndicatorMovingState);
        positionIndicatorMovingState->addTransition(this, &PlaybackViewModelContextData::positionIndicatorMoveWillFinish, idleState);

        idleState->addTransition(this, &PlaybackViewModelContextData::loopRangeTransactionWillStart, loopRangeAdjustPendingState);
        loopRangeAdjustPendingState->addTransition(this, &PlaybackViewModelContextData::loopRangeTransactionStarted, loopRangeAdjustingState);
        loopRangeAdjustPendingState->addTransition(this, &PlaybackViewModelContextData::loopRangeTransactionNotStarted, idleState);
        loopRangeAdjustingState->addTransition(this, &PlaybackViewModelContextData::loopRangeTransactionWillFinish, idleState);

        connect(idleState, &QState::entered, this, [=, this] {
            qCInfo(lcPlaybackViewModelContextData) << "Idle state entered";
        });
        connect(idleState, &QState::exited, this, [=, this] {
            qCInfo(lcPlaybackViewModelContextData) << "Idle state exited";
        });
        connect(rubberBandDraggingState, &QState::entered, this, [=, this] {
            qCInfo(lcPlaybackViewModelContextData) << "Rubber band dragging state entered";
        });
        connect(rubberBandDraggingState, &QState::exited, this, [=, this] {
            qCInfo(lcPlaybackViewModelContextData) << "Rubber band dragging state exited";
        });
        connect(positionIndicatorMovingState, &QState::entered, this, [=, this] {
            qCInfo(lcPlaybackViewModelContextData) << "Position indicator moving state entered";
            onPositionIndicatorMovingStateEntered();
        });
        connect(positionIndicatorMovingState, &QState::exited, this, [=, this] {
            qCInfo(lcPlaybackViewModelContextData) << "Position indicator moving state exited";
            onPositionIndicatorMovingStateExited();
        });
        connect(loopRangeAdjustPendingState, &QState::entered, this, [=, this] {
            qCInfo(lcPlaybackViewModelContextData) << "Loop range adjust pending state entered";
            onLoopRangeAdjustPendingStateEntered();
        });
        connect(loopRangeAdjustPendingState, &QState::exited, this, [=, this] {
            qCInfo(lcPlaybackViewModelContextData) << "Loop range adjust pending state exited";
        });
        connect(loopRangeAdjustingState, &QState::entered, this, [=, this] {
            qCInfo(lcPlaybackViewModelContextData) << "Loop range adjusting state entered";
        });
        connect(loopRangeAdjustingState, &QState::exited, this, [=, this] {
            qCInfo(lcPlaybackViewModelContextData) << "Loop range adjusting state exited";
            onLoopRangeAdjustingStateExited();
        });
    }

    void PlaybackViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        document = q->windowHandle()->projectDocumentContext()->document();
        timeline = document->model()->timeline();
        windowHandle = q->windowHandle();
        playbackViewModel = new sflow::PlaybackViewModel(q);

        initStateMachine();
    }

    void PlaybackViewModelContextData::bindPlaybackViewModel() {
        auto projectTimeline = windowHandle->projectTimeline();
        auto documentTimeline = timeline;
        QObject::connect(projectTimeline, &Core::ProjectTimeline::positionChanged, playbackViewModel, [=] {
            if (playbackViewModel->primaryPosition() == projectTimeline->position())
                return;
            qCDebug(lcPlaybackViewModelContextData) << "Project timeline position updated" << projectTimeline->position();
            playbackViewModel->setPrimaryPosition(projectTimeline->position());
        });
        QObject::connect(projectTimeline, &Core::ProjectTimeline::lastPositionChanged, playbackViewModel, [=] {
            if (playbackViewModel->secondaryPosition() == projectTimeline->lastPosition())
                return;
            qCDebug(lcPlaybackViewModelContextData) << "Project timeline last position updated" << projectTimeline->lastPosition();
            playbackViewModel->setSecondaryPosition(projectTimeline->lastPosition());
        });
        QObject::connect(playbackViewModel, &sflow::PlaybackViewModel::primaryPositionChanged, projectTimeline, [=] {
            if (projectTimeline->position() == playbackViewModel->primaryPosition())
                return;
            qCDebug(lcPlaybackViewModelContextData) << "Playback view model primary position updated" << playbackViewModel->primaryPosition();
            projectTimeline->setPosition(playbackViewModel->primaryPosition());
        });
        QObject::connect(playbackViewModel, &sflow::PlaybackViewModel::secondaryPositionChanged, projectTimeline, [=] {
            if (projectTimeline->lastPosition() == playbackViewModel->secondaryPosition())
                return;
            qCDebug(lcPlaybackViewModelContextData) << "Playback view model secondary position updated" << playbackViewModel->secondaryPosition();
            projectTimeline->setLastPosition(playbackViewModel->secondaryPosition());
        });
        playbackViewModel->setPrimaryPosition(projectTimeline->position());
        playbackViewModel->setSecondaryPosition(projectTimeline->lastPosition());

        QObject::connect(documentTimeline, &dspx::Timeline::loopStartChanged, playbackViewModel, [=] {
            const auto loopStart = documentTimeline->loopStart();
            if (playbackViewModel->loopStart() == loopStart)
                return;
            qCDebug(lcPlaybackViewModelContextData) << "Document timeline loop start updated" << loopStart;
            playbackViewModel->setLoopStart(loopStart);
        });
        QObject::connect(documentTimeline, &dspx::Timeline::loopLengthChanged, playbackViewModel, [=] {
            const int loopLength = documentTimeline->loopLength();
            const int targetLength = documentTimeline->isLoopEnabled() ? loopLength : -1;
            if (playbackViewModel->loopLength() == targetLength)
                return;
            qCDebug(lcPlaybackViewModelContextData) << "Document timeline loop length updated" << targetLength;
            playbackViewModel->setLoopLength(targetLength);
        });
        QObject::connect(documentTimeline, &dspx::Timeline::loopEnabledChanged, playbackViewModel, [=](bool enabled) {
            const int targetLength = enabled ? documentTimeline->loopLength() : -1;
            if (playbackViewModel->loopLength() == targetLength)
                return;
            qCDebug(lcPlaybackViewModelContextData) << "Document timeline loop enabled changed" << enabled << "-> length" << targetLength;
            playbackViewModel->setLoopLength(targetLength);
        });

        QObject::connect(playbackViewModel, &sflow::PlaybackViewModel::loopStartChanged, this, [=, this] {
            if (!stateMachine->configuration().contains(loopRangeAdjustingState)) {
                const int loopStart = documentTimeline->loopStart();
                if (playbackViewModel->loopStart() != loopStart) {
                    playbackViewModel->setLoopStart(loopStart);
                }
                return;
            }
            const int clampedLoopStart = std::max(0, playbackViewModel->loopStart());
            if (clampedLoopStart != playbackViewModel->loopStart()) {
                playbackViewModel->setLoopStart(clampedLoopStart);
            }
            qCDebug(lcPlaybackViewModelContextData) << "Loop view start updated" << clampedLoopStart;
            pendingLoopStart = clampedLoopStart;
        });

        QObject::connect(playbackViewModel, &sflow::PlaybackViewModel::loopLengthChanged, this, [=, this] {
            if (!stateMachine->configuration().contains(loopRangeAdjustingState)) {
                const int loopLength = documentTimeline->isLoopEnabled() ? documentTimeline->loopLength() : -1;
                if (playbackViewModel->loopLength() != loopLength) {
                    playbackViewModel->setLoopLength(loopLength);
                }
                return;
            }
            const int clampedLoopLength = std::max(1, playbackViewModel->loopLength());
            if (clampedLoopLength != playbackViewModel->loopLength()) {
                playbackViewModel->setLoopLength(clampedLoopLength);
            }
            qCDebug(lcPlaybackViewModelContextData) << "Loop view length updated" << clampedLoopLength;
            pendingLoopLength = clampedLoopLength;
        });

        playbackViewModel->setLoopStart(documentTimeline->loopStart());
        playbackViewModel->setLoopLength(documentTimeline->isLoopEnabled() ? documentTimeline->loopLength() : -1);
    }

    sflow::TimelineInteractionController *PlaybackViewModelContextData::createController(QObject *parent) {
        auto controller = new sflow::TimelineInteractionController(parent);
        controller->setInteraction(sflow::TimelineInteractionController::MovePositionIndicator | sflow::TimelineInteractionController::ZoomByRubberBand | sflow::TimelineInteractionController::AdjustLoopRange);

        connect(controller, &sflow::TimelineInteractionController::rubberBandDraggingStarted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillStart();
        });
        connect(controller, &sflow::TimelineInteractionController::rubberBandDraggingFinished, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillFinish();
        });
        connect(controller, &sflow::TimelineInteractionController::positionIndicatorMovingStarted, this, [=](QQuickItem *) {
            Q_EMIT positionIndicatorMoveWillStart();
        });
        connect(controller, &sflow::TimelineInteractionController::positionIndicatorMovingFinished, this, [=](QQuickItem *) {
            Q_EMIT positionIndicatorMoveWillFinish();
        });
        connect(controller, &sflow::TimelineInteractionController::loopRangeAdjustingStarted, this, [=](QQuickItem *) {
            Q_EMIT loopRangeTransactionWillStart();
        });
        connect(controller, &sflow::TimelineInteractionController::loopRangeAdjustingFinished, this, [=](QQuickItem *) {
            Q_EMIT loopRangeTransactionWillFinish();
        });

        return controller;
    }

    void PlaybackViewModelContextData::onLoopRangeAdjustPendingStateEntered() {
        loopRangeTransactionId = document->transactionController()->beginTransaction();
        if (loopRangeTransactionId != Core::TransactionController::TransactionId::Invalid) {
            pendingLoopStart = timeline->loopStart();
            pendingLoopLength = timeline->loopLength();
            Q_EMIT loopRangeTransactionStarted();
        } else {
            Q_EMIT loopRangeTransactionNotStarted();
        }
    }

    void PlaybackViewModelContextData::onPositionIndicatorMovingStateEntered() {
        // TODO
    }

    void PlaybackViewModelContextData::onPositionIndicatorMovingStateExited() {
        // TODO
    }

    void PlaybackViewModelContextData::onLoopRangeAdjustingStateExited() {
        if (loopRangeTransactionId == Core::TransactionController::TransactionId::Invalid) {
            return;
        }

        const bool hasChange = pendingLoopStart != timeline->loopStart() || pendingLoopLength != timeline->loopLength();
        if (hasChange) {
            timeline->setLoopStart(pendingLoopStart);
            timeline->setLoopLength(pendingLoopLength);
            document->transactionController()->commitTransaction(loopRangeTransactionId, tr("Adjust loop range"));
        } else {
            document->transactionController()->abortTransaction(loopRangeTransactionId);
        }

        loopRangeTransactionId = {};
        pendingLoopStart = {};
        pendingLoopLength = {};
    }

}
