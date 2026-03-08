#include "TrackViewModelContextData_p.h"

#include <QLoggingCategory>
#include <QQuickItem>
#include <QState>
#include <QStateMachine>
#include <QtGlobal>

#include <SVSCraftCore/DecibelLinearizer.h>

#include <ScopicFlowCore/ListViewModel.h>
#include <ScopicFlowCore/TrackListInteractionController.h>
#include <ScopicFlowCore/TrackViewModel.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackControl.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/TrackSelectionModel.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/Clip.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/PickTrackColorScenario.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/TrackColorSchema.h>

#include <visualeditor/private/TrackSelectionController_p.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcTrackViewModelContextData, "diffscope.visualeditor.trackviewmodelcontextdata")

    static inline double toDecibel(double gain) {
        return SVS::DecibelLinearizer::gainToDecibels(gain);
    }

    static inline double toLinear(double decibel) {
        return SVS::DecibelLinearizer::decibelsToGain(decibel);
    }

    static inline double getClampedHeight(dspx::Track *track) {
        return qMax(40.0, track->height());
    }

    void TrackViewModelContextData::initStateMachine() {
        stateMachine = new QStateMachine(QState::ExclusiveStates, this);

        idleState = new QState;
        rubberBandDraggingState = new QState;

        movePendingState = new QState;
        moveProcessingState = new QState;
        moveCommittingState = new QState;
        moveAbortingState = new QState;

        mutePendingState = new QState;
        muteEditingState = new QState;
        muteFinishingState = new QState;

        soloPendingState = new QState;
        soloEditingState = new QState;
        soloFinishingState = new QState;

        recordPendingState = new QState;
        recordEditingState = new QState;
        recordFinishingState = new QState;

        namePendingState = new QState;
        nameProgressingState = new QState;
        nameCommittingState = new QState;
        nameAbortingState = new QState;

        gainPendingState = new QState;
        gainProgressingState = new QState;
        gainCommittingState = new QState;
        gainAbortingState = new QState;

        panPendingState = new QState;
        panProgressingState = new QState;
        panCommittingState = new QState;
        panAbortingState = new QState;

        heightPendingState = new QState;
        heightEditingState = new QState;
        heightFinishingState = new QState;

        stateMachine->addState(idleState);
        stateMachine->addState(rubberBandDraggingState);

        stateMachine->addState(movePendingState);
        stateMachine->addState(moveProcessingState);
        stateMachine->addState(moveCommittingState);
        stateMachine->addState(moveAbortingState);

        stateMachine->addState(mutePendingState);
        stateMachine->addState(muteEditingState);
        stateMachine->addState(muteFinishingState);

        stateMachine->addState(soloPendingState);
        stateMachine->addState(soloEditingState);
        stateMachine->addState(soloFinishingState);

        stateMachine->addState(recordPendingState);
        stateMachine->addState(recordEditingState);
        stateMachine->addState(recordFinishingState);

        stateMachine->addState(namePendingState);
        stateMachine->addState(nameProgressingState);
        stateMachine->addState(nameCommittingState);
        stateMachine->addState(nameAbortingState);

        stateMachine->addState(gainPendingState);
        stateMachine->addState(gainProgressingState);
        stateMachine->addState(gainCommittingState);
        stateMachine->addState(gainAbortingState);

        stateMachine->addState(panPendingState);
        stateMachine->addState(panProgressingState);
        stateMachine->addState(panCommittingState);
        stateMachine->addState(panAbortingState);

        stateMachine->addState(heightPendingState);
        stateMachine->addState(heightEditingState);
        stateMachine->addState(heightFinishingState);

        stateMachine->setInitialState(idleState);
        stateMachine->start();

        idleState->addTransition(this, &TrackViewModelContextData::rubberBandDragWillStart, rubberBandDraggingState);
        rubberBandDraggingState->addTransition(this, &TrackViewModelContextData::rubberBandDragWillFinish, idleState);

        idleState->addTransition(this, &TrackViewModelContextData::moveTransactionWillStart, movePendingState);
        movePendingState->addTransition(this, &TrackViewModelContextData::moveTransactionStarted, moveProcessingState);
        movePendingState->addTransition(this, &TrackViewModelContextData::moveTransactionNotStarted, idleState);
        moveProcessingState->addTransition(this, &TrackViewModelContextData::moveTransactionWillCommit, moveCommittingState);
        moveProcessingState->addTransition(this, &TrackViewModelContextData::moveTransactionWillAbort, moveAbortingState);
        moveCommittingState->addTransition(idleState);
        moveAbortingState->addTransition(idleState);

        idleState->addTransition(this, &TrackViewModelContextData::muteTransactionWillStart, mutePendingState);
        mutePendingState->addTransition(this, &TrackViewModelContextData::muteTransactionStarted, muteEditingState);
        mutePendingState->addTransition(this, &TrackViewModelContextData::muteTransactionNotStarted, idleState);
        muteEditingState->addTransition(this, &TrackViewModelContextData::muteTransactionWillFinish, muteFinishingState);
        muteFinishingState->addTransition(idleState);

        idleState->addTransition(this, &TrackViewModelContextData::soloTransactionWillStart, soloPendingState);
        soloPendingState->addTransition(this, &TrackViewModelContextData::soloTransactionStarted, soloEditingState);
        soloPendingState->addTransition(this, &TrackViewModelContextData::soloTransactionNotStarted, idleState);
        soloEditingState->addTransition(this, &TrackViewModelContextData::soloTransactionWillFinish, soloFinishingState);
        soloFinishingState->addTransition(idleState);

        idleState->addTransition(this, &TrackViewModelContextData::recordTransactionWillStart, recordPendingState);
        recordPendingState->addTransition(this, &TrackViewModelContextData::recordTransactionStarted, recordEditingState);
        recordPendingState->addTransition(this, &TrackViewModelContextData::recordTransactionNotStarted, idleState);
        recordEditingState->addTransition(this, &TrackViewModelContextData::recordTransactionWillFinish, recordFinishingState);
        recordFinishingState->addTransition(idleState);

        idleState->addTransition(this, &TrackViewModelContextData::nameTransactionWillStart, namePendingState);
        namePendingState->addTransition(this, &TrackViewModelContextData::nameTransactionStarted, nameProgressingState);
        namePendingState->addTransition(this, &TrackViewModelContextData::nameTransactionNotStarted, idleState);
        nameProgressingState->addTransition(this, &TrackViewModelContextData::nameTransactionWillCommit, nameCommittingState);
        nameProgressingState->addTransition(this, &TrackViewModelContextData::nameTransactionWillAbort, nameAbortingState);
        nameCommittingState->addTransition(idleState);
        nameAbortingState->addTransition(idleState);

        idleState->addTransition(this, &TrackViewModelContextData::gainTransactionWillStart, gainPendingState);
        gainPendingState->addTransition(this, &TrackViewModelContextData::gainTransactionStarted, gainProgressingState);
        gainPendingState->addTransition(this, &TrackViewModelContextData::gainTransactionNotStarted, idleState);
        gainProgressingState->addTransition(this, &TrackViewModelContextData::gainTransactionWillCommit, gainCommittingState);
        gainProgressingState->addTransition(this, &TrackViewModelContextData::gainTransactionWillAbort, gainAbortingState);
        gainCommittingState->addTransition(idleState);
        gainAbortingState->addTransition(idleState);

        idleState->addTransition(this, &TrackViewModelContextData::panTransactionWillStart, panPendingState);
        panPendingState->addTransition(this, &TrackViewModelContextData::panTransactionStarted, panProgressingState);
        panPendingState->addTransition(this, &TrackViewModelContextData::panTransactionNotStarted, idleState);
        panProgressingState->addTransition(this, &TrackViewModelContextData::panTransactionWillCommit, panCommittingState);
        panProgressingState->addTransition(this, &TrackViewModelContextData::panTransactionWillAbort, panAbortingState);
        panCommittingState->addTransition(idleState);
        panAbortingState->addTransition(idleState);

        idleState->addTransition(this, &TrackViewModelContextData::heightTransactionWillStart, heightPendingState);
        heightPendingState->addTransition(this, &TrackViewModelContextData::heightTransactionStarted, heightEditingState);
        heightPendingState->addTransition(this, &TrackViewModelContextData::heightTransactionNotStarted, idleState);
        heightEditingState->addTransition(this, &TrackViewModelContextData::heightTransactionWillFinish, heightFinishingState);
        heightFinishingState->addTransition(idleState);

        auto logEntered = [](const char *name) { qCInfo(lcTrackViewModelContextData) << name << "entered"; };
        auto logExited = [](const char *name) { qCInfo(lcTrackViewModelContextData) << name << "exited"; };

        connect(idleState, &QState::entered, this, [=] {
            logEntered("Idle state");
        });
        connect(idleState, &QState::exited, this, [=] {
            logExited("Idle state");
        });
        connect(rubberBandDraggingState, &QState::entered, this, [=] {
            logEntered("Rubber band dragging state");
        });
        connect(rubberBandDraggingState, &QState::exited, this, [=] {
            logExited("Rubber band dragging state");
        });

        connect(movePendingState, &QState::entered, this, [=, this] {
            logEntered("Move pending state");
            onMovePendingStateEntered();
        });
        connect(movePendingState, &QState::exited, this, [=] {
            logExited("Move pending state");
        });
        connect(moveProcessingState, &QState::entered, this, [=] {
            logEntered("Move processing state");
        });
        connect(moveProcessingState, &QState::exited, this, [=] {
            logExited("Move processing state");
        });
        connect(moveCommittingState, &QState::entered, this, [=, this] {
            logEntered("Move committing state");
            onMoveCommittingStateEntered();
        });
        connect(moveCommittingState, &QState::exited, this, [=] {
            logExited("Move committing state");
        });
        connect(moveAbortingState, &QState::entered, this, [=, this] {
            logEntered("Move aborting state");
            onMoveAbortingStateEntered();
        });
        connect(moveAbortingState, &QState::exited, this, [=] {
            logExited("Move aborting state");
        });

        connect(mutePendingState, &QState::entered, this, [=, this] {
            logEntered("Mute pending state");
            onMutePendingStateEntered();
        });
        connect(mutePendingState, &QState::exited, this, [=] {
            logExited("Mute pending state");
        });
        connect(muteEditingState, &QState::entered, this, [=] {
            logEntered("Mute editing state");
        });
        connect(muteEditingState, &QState::exited, this, [=] {
            logExited("Mute editing state");
        });
        connect(muteFinishingState, &QState::entered, this, [=, this] {
            logEntered("Mute finishing state");
            onMuteFinishingStateEntered();
        });
        connect(muteFinishingState, &QState::exited, this, [=] {
            logExited("Mute finishing state");
        });

        connect(soloPendingState, &QState::entered, this, [=, this] {
            logEntered("Solo pending state");
            onSoloPendingStateEntered();
        });
        connect(soloPendingState, &QState::exited, this, [=] {
            logExited("Solo pending state");
        });
        connect(soloEditingState, &QState::entered, this, [=] {
            logEntered("Solo editing state");
        });
        connect(soloEditingState, &QState::exited, this, [=] {
            logExited("Solo editing state");
        });
        connect(soloFinishingState, &QState::entered, this, [=, this] {
            logEntered("Solo finishing state");
            onSoloFinishingStateEntered();
        });
        connect(soloFinishingState, &QState::exited, this, [=] {
            logExited("Solo finishing state");
        });

        connect(recordPendingState, &QState::entered, this, [=, this] {
            logEntered("Record pending state");
            onRecordPendingStateEntered();
        });
        connect(recordPendingState, &QState::exited, this, [=] {
            logExited("Record pending state");
        });
        connect(recordEditingState, &QState::entered, this, [=] {
            logEntered("Record editing state");
        });
        connect(recordEditingState, &QState::exited, this, [=] {
            logExited("Record editing state");
        });
        connect(recordFinishingState, &QState::entered, this, [=, this] {
            logEntered("Record finishing state");
            onRecordFinishingStateEntered();
        });
        connect(recordFinishingState, &QState::exited, this, [=] {
            logExited("Record finishing state");
        });

        connect(namePendingState, &QState::entered, this, [=, this] {
            logEntered("Name pending state");
            onNamePendingStateEntered();
        });
        connect(namePendingState, &QState::exited, this, [=] {
            logExited("Name pending state");
        });
        connect(nameProgressingState, &QState::entered, this, [=] {
            logEntered("Name progressing state");
        });
        connect(nameProgressingState, &QState::exited, this, [=] {
            logExited("Name progressing state");
        });
        connect(nameCommittingState, &QState::entered, this, [=, this] {
            logEntered("Name committing state");
            onNameCommittingStateEntered();
        });
        connect(nameCommittingState, &QState::exited, this, [=] {
            logExited("Name committing state");
        });
        connect(nameAbortingState, &QState::entered, this, [=, this] {
            logEntered("Name aborting state");
            onNameAbortingStateEntered();
        });
        connect(nameAbortingState, &QState::exited, this, [=] {
            logExited("Name aborting state");
        });

        connect(gainPendingState, &QState::entered, this, [=, this] {
            logEntered("Gain pending state");
            onGainPendingStateEntered();
        });
        connect(gainPendingState, &QState::exited, this, [=] {
            logExited("Gain pending state");
        });
        connect(gainProgressingState, &QState::entered, this, [=] {
            logEntered("Gain progressing state");
        });
        connect(gainProgressingState, &QState::exited, this, [=] {
            logExited("Gain progressing state");
        });
        connect(gainCommittingState, &QState::entered, this, [=, this] {
            logEntered("Gain committing state");
            onGainCommittingStateEntered();
        });
        connect(gainCommittingState, &QState::exited, this, [=] {
            logExited("Gain committing state");
        });
        connect(gainAbortingState, &QState::entered, this, [=, this] {
            logEntered("Gain aborting state");
            onGainAbortingStateEntered();
        });
        connect(gainAbortingState, &QState::exited, this, [=] {
            logExited("Gain aborting state");
        });

        connect(panPendingState, &QState::entered, this, [=, this] {
            logEntered("Pan pending state");
            onPanPendingStateEntered();
        });
        connect(panPendingState, &QState::exited, this, [=] {
            logExited("Pan pending state");
        });
        connect(panProgressingState, &QState::entered, this, [=] {
            logEntered("Pan progressing state");
        });
        connect(panProgressingState, &QState::exited, this, [=] {
            logExited("Pan progressing state");
        });
        connect(panCommittingState, &QState::entered, this, [=, this] {
            logEntered("Pan committing state");
            onPanCommittingStateEntered();
        });
        connect(panCommittingState, &QState::exited, this, [=] {
            logExited("Pan committing state");
        });
        connect(panAbortingState, &QState::entered, this, [=, this] {
            logEntered("Pan aborting state");
            onPanAbortingStateEntered();
        });
        connect(panAbortingState, &QState::exited, this, [=] {
            logExited("Pan aborting state");
        });

        connect(heightPendingState, &QState::entered, this, [=, this] {
            logEntered("Height pending state");
            onHeightPendingStateEntered();
        });
        connect(heightPendingState, &QState::exited, this, [=] {
            logExited("Height pending state");
        });
        connect(heightEditingState, &QState::entered, this, [=] {
            logEntered("Height editing state");
        });
        connect(heightEditingState, &QState::exited, this, [=] {
            logExited("Height editing state");
        });
        connect(heightFinishingState, &QState::entered, this, [=, this] {
            logEntered("Height finishing state");
            onHeightFinishingStateEntered();
        });
        connect(heightFinishingState, &QState::exited, this, [=] {
            logExited("Height finishing state");
        });
    }

    void TrackViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        document = q->windowHandle()->projectDocumentContext()->document();
        trackList = document->model()->tracks();
        trackSelectionModel = document->selectionModel()->trackSelectionModel();

        trackColorSchema = Core::CoreInterface::trackColorSchema();
        connect(trackColorSchema, &Core::TrackColorSchema::colorsChanged, this, [=, this](const QList<QColor> &) {
            updateAllTrackColors();
        });

        trackListViewModel = new sflow::ListViewModel(q);
        trackSelectionController = new TrackSelectionController(q);

        initStateMachine();
    }

    void TrackViewModelContextData::bindTrackListViewModel() {
        connect(trackList, &dspx::TrackList::itemInserted, trackListViewModel, [=, this](int index, dspx::Track *item) {
            bindTrackDocumentItem(index, item);
        });
        connect(trackList, &dspx::TrackList::itemRemoved, trackListViewModel, [=, this](int index, dspx::Track *item) {
            unbindTrackDocumentItem(index, item);
        });
        connect(trackList, &dspx::TrackList::rotated, this, [=, this](int leftIndex, int middleIndex, int rightIndex) {
            if (stateMachine->configuration().contains(moveProcessingState)) {
                return;
            }
            trackListViewModel->rotate(leftIndex, middleIndex, rightIndex);
        });

        connect(trackListViewModel, &sflow::ListViewModel::rotated, this, [=, this](int leftIndex, int middleIndex, int rightIndex) {
            if (!stateMachine->configuration().contains(moveProcessingState)) {
                return;
            }
            moveChanged = true;
            trackList->rotate(leftIndex, middleIndex, rightIndex);
        });

        const auto &items = trackList->items();
        for (int i = 0; i < items.size(); ++i) {
            bindTrackDocumentItem(i, items.at(i));
        }

        connect(trackSelectionModel, &dspx::TrackSelectionModel::itemSelected, this, [=, this](dspx::Track *item, bool selected) {
            auto viewItem = trackViewItemMap.value(item);
            Q_ASSERT(viewItem);
            viewItem->setSelected(selected);
        });

        for (auto *selectedItem : trackSelectionModel->selectedItems()) {
            auto viewItem = trackViewItemMap.value(selectedItem);
            if (!viewItem) {
                continue;
            }
            viewItem->setSelected(true);
        }
    }

    void TrackViewModelContextData::bindTrackDocumentItem(int index, dspx::Track *item) {
        if (trackViewItemMap.contains(item)) {
            return;
        }

        auto viewItem = new sflow::TrackViewModel(trackListViewModel);
        trackViewItemMap.insert(item, viewItem);
        trackDocumentItemMap.insert(viewItem, item);

        auto control = item->control();

        connect(item, &dspx::Track::colorIdChanged, viewItem, [=, this](int colorId) {
            const QColor color = trackColorForId(colorId);
            if (viewItem->color() == color) {
                return;
            }
            viewItem->setColor(color);
        });
        connect(item, &dspx::Track::nameChanged, viewItem, [=] {
            if (viewItem->name() == item->name()) {
                return;
            }
            viewItem->setName(item->name());
        });
        connect(item, &dspx::Track::heightChanged, viewItem, [=] {
            if (viewItem->rowHeight() == getClampedHeight(item)) {
                return;
            }
            viewItem->setRowHeight(getClampedHeight(item));
        });
        connect(control, &dspx::TrackControl::muteChanged, viewItem, [=](bool mute) {
            if (viewItem->isMute() == mute) {
                return;
            }
            viewItem->setMute(mute);
        });
        connect(control, &dspx::TrackControl::soloChanged, viewItem, [=](bool solo) {
            if (viewItem->isSolo() == solo) {
                return;
            }
            viewItem->setSolo(solo);
        });
        connect(control, &dspx::TrackControl::recordChanged, viewItem, [=](bool record) {
            if (viewItem->isRecord() == record) {
                return;
            }
            viewItem->setRecord(record);
        });
        connect(control, &dspx::TrackControl::gainChanged, viewItem, [=](double gain) {
            const double db = toDecibel(gain);
            if (viewItem->gain() == db) {
                return;
            }
            viewItem->setGain(db);
        });
        connect(control, &dspx::TrackControl::panChanged, viewItem, [=](double pan) {
            if (viewItem->pan() == pan) {
                return;
            }
            viewItem->setPan(pan);
        });

        connect(viewItem, &sflow::TrackViewModel::nameChanged, item, [=] {
            if (!stateMachine->configuration().contains(nameProgressingState)) {
                viewItem->setName(item->name());
                return;
            }
        });
        connect(viewItem, &sflow::TrackViewModel::muteChanged, item, [=] {
            if (!stateMachine->configuration().contains(muteEditingState)) {
                viewItem->setMute(control->mute());
                return;
            }
        });
        connect(viewItem, &sflow::TrackViewModel::soloChanged, item, [=] {
            if (!stateMachine->configuration().contains(soloEditingState)) {
                viewItem->setSolo(control->solo());
                return;
            }
        });
        connect(viewItem, &sflow::TrackViewModel::recordChanged, item, [=] {
            if (!stateMachine->configuration().contains(recordEditingState)) {
                viewItem->setRecord(control->record());
                return;
            }
        });
        connect(viewItem, &sflow::TrackViewModel::gainChanged, item, [=] {
            if (!stateMachine->configuration().contains(gainProgressingState)) {
                viewItem->setGain(toDecibel(control->gain()));
                return;
            }
        });
        connect(viewItem, &sflow::TrackViewModel::panChanged, item, [=] {
            if (!stateMachine->configuration().contains(panProgressingState)) {
                viewItem->setPan(control->pan());
                return;
            }
        });
        connect(viewItem, &sflow::TrackViewModel::rowHeightChanged, item, [=] {
            if (!stateMachine->configuration().contains(heightEditingState)) {
                viewItem->setRowHeight(getClampedHeight(item));
                return;
            }
        });

        viewItem->setName(item->name());
        viewItem->setRowHeight(getClampedHeight(item));
        viewItem->setMute(control->mute());
        viewItem->setSolo(control->solo());
        viewItem->setRecord(control->record());
        viewItem->setGain(toDecibel(control->gain()));
        viewItem->setPan(control->pan());
        viewItem->setColor(trackColorForId(item->colorId()));

        trackListViewModel->insertItem(index, viewItem);
    }

    void TrackViewModelContextData::unbindTrackDocumentItem(int index, dspx::Track *item) {
        Q_UNUSED(index)
        if (!trackViewItemMap.contains(item)) {
            return;
        }
        auto viewItem = trackViewItemMap.take(item);
        trackDocumentItemMap.remove(viewItem);

        disconnect(item, nullptr, viewItem, nullptr);
        disconnect(viewItem, nullptr, item, nullptr);

        trackListViewModel->removeItem(trackListViewModel->items().indexOf(viewItem));

        viewItem->deleteLater();
    }

    sflow::TrackListInteractionController *TrackViewModelContextData::createController(QObject *parent) {
        auto controller = new sflow::TrackListInteractionController(parent);
        controller->setPrimarySceneInteraction(sflow::TrackListInteractionController::RubberBandSelect);
        controller->setSecondarySceneInteraction(sflow::TrackListInteractionController::RubberBandSelect);
        controller->setPrimarySelectInteraction(sflow::TrackListInteractionController::RubberBandSelect);
        controller->setSecondarySelectInteraction(sflow::TrackListInteractionController::RubberBandSelect);
        controller->setPrimaryItemInteraction(sflow::TrackListInteractionController::DragMove);
        controller->setSecondaryItemInteraction(sflow::TrackListInteractionController::DragCopy);
        controller->setItemAction(sflow::TrackListInteractionController::EditMute | sflow::TrackListInteractionController::EditSolo | sflow::TrackListInteractionController::EditRecord | sflow::TrackListInteractionController::EditName | sflow::TrackListInteractionController::EditGain | sflow::TrackListInteractionController::EditPan | sflow::TrackListInteractionController::AdjustHeight);

        connect(controller, &sflow::TrackListInteractionController::rubberBandDraggingStarted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::rubberBandDraggingCommitted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillFinish();
        });
        connect(controller, &sflow::TrackListInteractionController::rubberBandDraggingAborted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillFinish();
        });

        connect(controller, &sflow::TrackListInteractionController::dragMovingStarted, this, [=](QQuickItem *, int index) {
            targetTrack = trackList->item(index);
            Q_EMIT moveTransactionWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::dragMovingCommitted, this, [=](QQuickItem *, int) {
            Q_EMIT moveTransactionWillCommit();
        });
        connect(controller, &sflow::TrackListInteractionController::dragMovingAborted, this, [=](QQuickItem *, int) {
            Q_EMIT moveTransactionWillAbort();
        });

        connect(controller, &sflow::TrackListInteractionController::muteEditingStarted, this, [=](QQuickItem *, int index) {
            targetTrack = trackList->item(index);
            Q_EMIT muteTransactionWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::muteEditingFinished, this, [=](QQuickItem *, int) {
            Q_EMIT muteTransactionWillFinish();
        });

        connect(controller, &sflow::TrackListInteractionController::soloEditingStarted, this, [=](QQuickItem *, int index) {
            targetTrack = trackList->item(index);
            Q_EMIT soloTransactionWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::soloEditingFinished, this, [=](QQuickItem *, int) {
            Q_EMIT soloTransactionWillFinish();
        });

        connect(controller, &sflow::TrackListInteractionController::recordEditingStarted, this, [=](QQuickItem *, int index) {
            targetTrack = trackList->item(index);
            Q_EMIT recordTransactionWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::recordEditingFinished, this, [=](QQuickItem *, int) {
            Q_EMIT recordTransactionWillFinish();
        });

        connect(controller, &sflow::TrackListInteractionController::nameEditingStarted, this, [=](QQuickItem *, int index) {
            targetTrack = trackList->item(index);
            Q_EMIT nameTransactionWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::nameEditingCommitted, this, [=](QQuickItem *, int) {
            Q_EMIT nameTransactionWillCommit();
        });
        connect(controller, &sflow::TrackListInteractionController::nameEditingAborted, this, [=](QQuickItem *, int) {
            Q_EMIT nameTransactionWillAbort();
        });

        connect(controller, &sflow::TrackListInteractionController::gainEditingStarted, this, [=](QQuickItem *, int index) {
            targetTrack = trackList->item(index);
            Q_EMIT gainTransactionWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::gainEditingCommitted, this, [=](QQuickItem *, int) {
            Q_EMIT gainTransactionWillCommit();
        });
        connect(controller, &sflow::TrackListInteractionController::gainEditingAborted, this, [=](QQuickItem *, int) {
            Q_EMIT gainTransactionWillAbort();
        });

        connect(controller, &sflow::TrackListInteractionController::panEditingStarted, this, [=](QQuickItem *, int index) {
            targetTrack = trackList->item(index);
            Q_EMIT panTransactionWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::panEditingCommitted, this, [=](QQuickItem *, int) {
            Q_EMIT panTransactionWillCommit();
        });
        connect(controller, &sflow::TrackListInteractionController::panEditingAborted, this, [=](QQuickItem *, int) {
            Q_EMIT panTransactionWillAbort();
        });

        connect(controller, &sflow::TrackListInteractionController::heightAdjustingStarted, this, [=](QQuickItem *, int index) {
            targetTrack = trackList->item(index);
            Q_EMIT heightTransactionWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::heightAdjustingFinished, this, [=](QQuickItem *, int) {
            Q_EMIT heightTransactionWillFinish();
        });

        connect(controller, &sflow::TrackListInteractionController::itemColorIndicatorClicked, this, [=](QQuickItem *trackListItem, int index) {
            qCDebug(lcTrackViewModelContextData) << "Track color indicator clicked" << index;
            onItemColorIndicatorClicked(trackListItem, index);
        });

        connect(controller, &sflow::TrackListInteractionController::itemDoubleClicked, this, [=](QQuickItem *, int index) {
            qCDebug(lcTrackViewModelContextData) << "Track item double clicked" << index;
            selectAllClipsOnTrack(index);
        });

        return controller;
    }

    void TrackViewModelContextData::onMovePendingStateEntered() {
        moveTransactionId = document->transactionController()->beginTransaction();
        moveChanged = false;
        if (moveTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT moveTransactionStarted();
        } else {
            Q_EMIT moveTransactionNotStarted();
        }
    }

    void TrackViewModelContextData::onMoveCommittingStateEntered() {
        if (!moveChanged) {
            document->transactionController()->abortTransaction(moveTransactionId);
        } else {
            document->transactionController()->commitTransaction(moveTransactionId, tr("Moving track"));
        }
        moveTransactionId = {};
        targetTrack = {};
        moveChanged = false;
    }

    void TrackViewModelContextData::onMoveAbortingStateEntered() {
        document->transactionController()->abortTransaction(moveTransactionId);
        moveTransactionId = {};
        targetTrack = {};
        moveChanged = false;
    }

    void TrackViewModelContextData::onMutePendingStateEntered() {
        muteTransactionId = document->transactionController()->beginTransaction();
        if (muteTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT muteTransactionStarted();
        } else {
            Q_EMIT muteTransactionNotStarted();
        }
    }

    void TrackViewModelContextData::onMuteFinishingStateEntered() {
        if (!targetTrack || muteTransactionId == Core::TransactionController::TransactionId::Invalid) {
            targetTrack = {};
            return;
        }
        auto viewItem = trackViewItemMap.value(targetTrack);
        Q_ASSERT(viewItem);
        const bool newValue = viewItem->isMute();
        if (newValue == targetTrack->control()->mute()) {
            document->transactionController()->abortTransaction(muteTransactionId);
        } else {
            targetTrack->control()->setMute(newValue);
            document->transactionController()->commitTransaction(muteTransactionId, tr("Editing track mute"));
        }
        muteTransactionId = {};
        targetTrack = {};
    }

    void TrackViewModelContextData::onSoloPendingStateEntered() {
        soloTransactionId = document->transactionController()->beginTransaction();
        if (soloTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT soloTransactionStarted();
        } else {
            Q_EMIT soloTransactionNotStarted();
        }
    }

    void TrackViewModelContextData::onSoloFinishingStateEntered() {
        if (!targetTrack || soloTransactionId == Core::TransactionController::TransactionId::Invalid) {
            targetTrack = {};
            return;
        }
        auto viewItem = trackViewItemMap.value(targetTrack);
        Q_ASSERT(viewItem);
        const bool newValue = viewItem->isSolo();
        if (newValue == targetTrack->control()->solo()) {
            document->transactionController()->abortTransaction(soloTransactionId);
        } else {
            targetTrack->control()->setSolo(newValue);
            document->transactionController()->commitTransaction(soloTransactionId, tr("Editing track solo"));
        }
        soloTransactionId = {};
        targetTrack = {};
    }

    void TrackViewModelContextData::onRecordPendingStateEntered() {
        recordTransactionId = document->transactionController()->beginTransaction();
        if (recordTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT recordTransactionStarted();
        } else {
            Q_EMIT recordTransactionNotStarted();
        }
    }

    void TrackViewModelContextData::onRecordFinishingStateEntered() {
        if (!targetTrack || recordTransactionId == Core::TransactionController::TransactionId::Invalid) {
            targetTrack = {};
            return;
        }
        auto viewItem = trackViewItemMap.value(targetTrack);
        Q_ASSERT(viewItem);
        const bool newValue = viewItem->isRecord();
        if (newValue == targetTrack->control()->record()) {
            document->transactionController()->abortTransaction(recordTransactionId);
        } else {
            targetTrack->control()->setRecord(newValue);
            document->transactionController()->commitTransaction(recordTransactionId, tr("Editing track record"));
        }
        recordTransactionId = {};
        targetTrack = {};
    }

    void TrackViewModelContextData::onNamePendingStateEntered() {
        nameTransactionId = document->transactionController()->beginTransaction();
        if (nameTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT nameTransactionStarted();
        } else {
            Q_EMIT nameTransactionNotStarted();
        }
    }

    void TrackViewModelContextData::onNameCommittingStateEntered() {
        if (!targetTrack || nameTransactionId == Core::TransactionController::TransactionId::Invalid) {
            targetTrack = {};
            return;
        }
        auto viewItem = trackViewItemMap.value(targetTrack);
        Q_ASSERT(viewItem);
        if (viewItem->name() == targetTrack->name()) {
            document->transactionController()->abortTransaction(nameTransactionId);
        } else {
            targetTrack->setName(viewItem->name());
            document->transactionController()->commitTransaction(nameTransactionId, tr("Renaming track"));
        }
        nameTransactionId = {};
        targetTrack = {};
    }

    void TrackViewModelContextData::onNameAbortingStateEntered() {
        document->transactionController()->abortTransaction(nameTransactionId);
        nameTransactionId = {};
        targetTrack = {};
    }

    void TrackViewModelContextData::onGainPendingStateEntered() {
        gainTransactionId = document->transactionController()->beginTransaction();
        if (gainTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT gainTransactionStarted();
        } else {
            Q_EMIT gainTransactionNotStarted();
        }
    }

    void TrackViewModelContextData::onGainCommittingStateEntered() {
        if (!targetTrack || gainTransactionId == Core::TransactionController::TransactionId::Invalid) {
            targetTrack = {};
            return;
        }
        auto viewItem = trackViewItemMap.value(targetTrack);
        Q_ASSERT(viewItem);
        const double newLinear = toLinear(viewItem->gain());
        if (qFuzzyCompare(newLinear, targetTrack->control()->gain())) {
            document->transactionController()->abortTransaction(gainTransactionId);
        } else {
            targetTrack->control()->setGain(newLinear);
            document->transactionController()->commitTransaction(gainTransactionId, tr("Adjusting track gain"));
        }
        gainTransactionId = {};
        targetTrack = {};
    }

    void TrackViewModelContextData::onGainAbortingStateEntered() {
        document->transactionController()->abortTransaction(gainTransactionId);
        gainTransactionId = {};
        targetTrack = {};
    }

    void TrackViewModelContextData::onPanPendingStateEntered() {
        panTransactionId = document->transactionController()->beginTransaction();
        if (panTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT panTransactionStarted();
        } else {
            Q_EMIT panTransactionNotStarted();
        }
    }

    void TrackViewModelContextData::onPanCommittingStateEntered() {
        if (!targetTrack || panTransactionId == Core::TransactionController::TransactionId::Invalid) {
            targetTrack = {};
            return;
        }
        auto viewItem = trackViewItemMap.value(targetTrack);
        Q_ASSERT(viewItem);
        const double newPan = viewItem->pan();
        if (qFuzzyCompare(newPan, targetTrack->control()->pan())) {
            document->transactionController()->abortTransaction(panTransactionId);
        } else {
            targetTrack->control()->setPan(newPan);
            document->transactionController()->commitTransaction(panTransactionId, tr("Adjusting track pan"));
        }
        panTransactionId = {};
        targetTrack = {};
    }

    void TrackViewModelContextData::onPanAbortingStateEntered() {
        document->transactionController()->abortTransaction(panTransactionId);
        panTransactionId = {};
        targetTrack = {};
    }

    void TrackViewModelContextData::onHeightPendingStateEntered() {
        heightTransactionId = document->transactionController()->beginTransaction();
        if (heightTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT heightTransactionStarted();
        } else {
            Q_EMIT heightTransactionNotStarted();
        }
    }

    void TrackViewModelContextData::onHeightFinishingStateEntered() {
        if (!targetTrack || heightTransactionId == Core::TransactionController::TransactionId::Invalid) {
            targetTrack = {};
            return;
        }
        auto viewItem = trackViewItemMap.value(targetTrack);
        Q_ASSERT(viewItem);
        const double newHeight = viewItem->rowHeight();
        if (qFuzzyCompare(newHeight, getClampedHeight(targetTrack))) {
            document->transactionController()->abortTransaction(heightTransactionId);
        } else {
            targetTrack->setHeight(newHeight);
            document->transactionController()->commitTransaction(heightTransactionId, tr("Resizing track"));
        }
        heightTransactionId = {};
        targetTrack = {};
    }

    void TrackViewModelContextData::onItemColorIndicatorClicked(QQuickItem *trackListItem, int index) {
        if (!trackListItem) {
            return;
        }

        auto track = trackList->item(index);
        if (!track) {
            return;
        }

        Core::PickTrackColorScenario scenario;
        scenario.setDocument(document);
        scenario.setWindow(trackListItem->window());
        scenario.setShouldDialogPopupAtCursor(true);
        scenario.pickTrackColor(track);
    }

    QColor TrackViewModelContextData::trackColorForId(int colorId) const {
        const auto colors = trackColorSchema->colors();
        if (colors.isEmpty()) {
            return QColor();
        }

        const int paletteSize = colors.size();
        int normalizedId = colorId % paletteSize;
        if (normalizedId < 0) {
            normalizedId += paletteSize;
        }
        return colors.at(normalizedId);
    }

    void TrackViewModelContextData::updateAllTrackColors() {
        for (auto it = trackViewItemMap.cbegin(); it != trackViewItemMap.cend(); ++it) {
            auto track = it.key();
            auto viewItem = it.value();
            viewItem->setColor(trackColorForId(track->colorId()));
        }
    }

    void TrackViewModelContextData::selectAllClipsOnTrack(int index) {
        auto track = trackList->item(index);
        document->selectionModel()->select(nullptr, dspx::SelectionModel::ClearPreviousSelection);
        for (auto clip : track->clips()->asRange()) {
            document->selectionModel()->select(clip, dspx::SelectionModel::Select);
        }
    }

}
