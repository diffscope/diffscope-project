#include "PlaybackViewModelContextData_p.h"

#include <QLoggingCategory>
#include <QQuickItem>
#include <QState>
#include <QStateMachine>

#include <ScopicFlowCore/PlaybackViewModel.h>
#include <ScopicFlowCore/TimelineInteractionController.h>

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
        stateMachine->addState(idleState);
        stateMachine->addState(rubberBandDraggingState);
        stateMachine->addState(positionIndicatorMovingState);
        stateMachine->setInitialState(idleState);
        stateMachine->start();

        idleState->addTransition(this, &PlaybackViewModelContextData::rubberBandDragWillStart, rubberBandDraggingState);
        rubberBandDraggingState->addTransition(this, &PlaybackViewModelContextData::rubberBandDragWillFinish, idleState);

        idleState->addTransition(this, &PlaybackViewModelContextData::positionIndicatorMoveWillStart, positionIndicatorMovingState);
        positionIndicatorMovingState->addTransition(this, &PlaybackViewModelContextData::positionIndicatorMoveWillFinish, idleState);

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
    }

    void PlaybackViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        windowHandle = q->windowHandle();
        playbackViewModel = new sflow::PlaybackViewModel(q);

        initStateMachine();
    }

    void PlaybackViewModelContextData::bindPlaybackViewModel() {
        auto projectTimeline = windowHandle->projectTimeline();
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
    }

    sflow::TimelineInteractionController *PlaybackViewModelContextData::createController(QObject *parent) {
        auto controller = new sflow::TimelineInteractionController(parent);
        controller->setInteraction(sflow::TimelineInteractionController::MovePositionIndicator | sflow::TimelineInteractionController::ZoomByRubberBand);

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

        return controller;
    }

    void PlaybackViewModelContextData::onPositionIndicatorMovingStateEntered() {
        // TODO
    }

    void PlaybackViewModelContextData::onPositionIndicatorMovingStateExited() {
        // TODO
    }

}
