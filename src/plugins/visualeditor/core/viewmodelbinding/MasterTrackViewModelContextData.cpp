#include "MasterTrackViewModelContextData_p.h"

#include <QColor>
#include <QLoggingCategory>
#include <QState>
#include <QStateMachine>
#include <QtGlobal>

#include <ScopicFlowCore/ListViewModel.h>
#include <ScopicFlowCore/TrackListInteractionController.h>
#include <ScopicFlowCore/TrackViewModel.h>

#include <SVSCraftCore/DecibelLinearizer.h>

#include <dspxmodel/BusControl.h>
#include <dspxmodel/Control.h>
#include <dspxmodel/Master.h>
#include <dspxmodel/Model.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcMasterTrackViewModelContextData, "diffscope.visualeditor.mastertrackviewmodelcontextdata")

    static inline double toDecibel(double gain) {
        return SVS::DecibelLinearizer::gainToDecibels(gain);
    }

    static inline double toLinear(double decibel) {
        return SVS::DecibelLinearizer::decibelsToGain(decibel);
    }

    void MasterTrackViewModelContextData::initStateMachine() {
        stateMachine = new QStateMachine(QState::ExclusiveStates, this);

        idleState = new QState;

        mutePendingState = new QState;
        muteEditingState = new QState;
        muteFinishingState = new QState;

        gainPendingState = new QState;
        gainProgressingState = new QState;
        gainCommittingState = new QState;
        gainAbortingState = new QState;

        panPendingState = new QState;
        panProgressingState = new QState;
        panCommittingState = new QState;
        panAbortingState = new QState;

        multiChannelPendingState = new QState;
        multiChannelEditingState = new QState;
        multiChannelFinishingState = new QState;

        stateMachine->addState(idleState);

        stateMachine->addState(mutePendingState);
        stateMachine->addState(muteEditingState);
        stateMachine->addState(muteFinishingState);

        stateMachine->addState(gainPendingState);
        stateMachine->addState(gainProgressingState);
        stateMachine->addState(gainCommittingState);
        stateMachine->addState(gainAbortingState);

        stateMachine->addState(panPendingState);
        stateMachine->addState(panProgressingState);
        stateMachine->addState(panCommittingState);
        stateMachine->addState(panAbortingState);

        stateMachine->addState(multiChannelPendingState);
        stateMachine->addState(multiChannelEditingState);
        stateMachine->addState(multiChannelFinishingState);

        stateMachine->setInitialState(idleState);
        stateMachine->start();

        idleState->addTransition(this, &MasterTrackViewModelContextData::muteTransactionWillStart, mutePendingState);
        mutePendingState->addTransition(this, &MasterTrackViewModelContextData::muteTransactionStarted, muteEditingState);
        mutePendingState->addTransition(this, &MasterTrackViewModelContextData::muteTransactionNotStarted, idleState);
        muteEditingState->addTransition(this, &MasterTrackViewModelContextData::muteTransactionWillFinish, muteFinishingState);
        muteFinishingState->addTransition(idleState);

        idleState->addTransition(this, &MasterTrackViewModelContextData::gainTransactionWillStart, gainPendingState);
        gainPendingState->addTransition(this, &MasterTrackViewModelContextData::gainTransactionStarted, gainProgressingState);
        gainPendingState->addTransition(this, &MasterTrackViewModelContextData::gainTransactionNotStarted, idleState);
        gainProgressingState->addTransition(this, &MasterTrackViewModelContextData::gainTransactionWillCommit, gainCommittingState);
        gainProgressingState->addTransition(this, &MasterTrackViewModelContextData::gainTransactionWillAbort, gainAbortingState);
        gainCommittingState->addTransition(idleState);
        gainAbortingState->addTransition(idleState);

        idleState->addTransition(this, &MasterTrackViewModelContextData::panTransactionWillStart, panPendingState);
        panPendingState->addTransition(this, &MasterTrackViewModelContextData::panTransactionStarted, panProgressingState);
        panPendingState->addTransition(this, &MasterTrackViewModelContextData::panTransactionNotStarted, idleState);
        panProgressingState->addTransition(this, &MasterTrackViewModelContextData::panTransactionWillCommit, panCommittingState);
        panProgressingState->addTransition(this, &MasterTrackViewModelContextData::panTransactionWillAbort, panAbortingState);
        panCommittingState->addTransition(idleState);
        panAbortingState->addTransition(idleState);

        idleState->addTransition(this, &MasterTrackViewModelContextData::multiChannelTransactionWillStart, multiChannelPendingState);
        multiChannelPendingState->addTransition(this, &MasterTrackViewModelContextData::multiChannelTransactionStarted, multiChannelEditingState);
        multiChannelPendingState->addTransition(this, &MasterTrackViewModelContextData::multiChannelTransactionNotStarted, idleState);
        multiChannelEditingState->addTransition(this, &MasterTrackViewModelContextData::multiChannelTransactionWillFinish, multiChannelFinishingState);
        multiChannelFinishingState->addTransition(idleState);

        auto logEntered = [](const char *name) { qCInfo(lcMasterTrackViewModelContextData) << name << "entered"; };
        auto logExited = [](const char *name) { qCInfo(lcMasterTrackViewModelContextData) << name << "exited"; };

        connect(idleState, &QState::entered, this, [=] {
            logEntered("Idle state");
        });
        connect(idleState, &QState::exited, this, [=] {
            logExited("Idle state");
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

        connect(multiChannelPendingState, &QState::entered, this, [=, this] {
            logEntered("Multi-channel pending state");
            onMultiChannelPendingStateEntered();
        });
        connect(multiChannelPendingState, &QState::exited, this, [=] {
            logExited("Multi-channel pending state");
        });
        connect(multiChannelEditingState, &QState::entered, this, [=] {
            logEntered("Multi-channel editing state");
        });
        connect(multiChannelEditingState, &QState::exited, this, [=] {
            logExited("Multi-channel editing state");
        });
        connect(multiChannelFinishingState, &QState::entered, this, [=, this] {
            logEntered("Multi-channel finishing state");
            onMultiChannelFinishingStateEntered();
        });
        connect(multiChannelFinishingState, &QState::exited, this, [=] {
            logExited("Multi-channel finishing state");
        });
    }

    void MasterTrackViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        document = q->windowHandle()->projectDocumentContext()->document();
        master = document->model()->master();

        masterTrackListViewModel = new sflow::ListViewModel(q);
        masterTrackViewModel = new sflow::TrackViewModel(masterTrackListViewModel);

        initStateMachine();
    }

    void MasterTrackViewModelContextData::bindMasterTrackViewModel() {
        auto control = master->control();

        connect(control, &dspx::Control::muteChanged, masterTrackViewModel, [=](bool mute) {
            if (masterTrackViewModel->isMute() == mute) {
                return;
            }
            masterTrackViewModel->setMute(mute);
        });
        connect(control, &dspx::Control::gainChanged, masterTrackViewModel, [=](double gain) {
            const double db = toDecibel(gain);
            if (qFuzzyCompare(masterTrackViewModel->gain(), db)) {
                return;
            }
            masterTrackViewModel->setGain(db);
        });
        connect(control, &dspx::Control::panChanged, masterTrackViewModel, [=](double pan) {
            if (qFuzzyCompare(masterTrackViewModel->pan(), pan)) {
                return;
            }
            masterTrackViewModel->setPan(pan);
        });
        connect(master, &dspx::Master::multiChannelOutputChanged, this, [=](bool enabled) {
            if (masterTrackViewModel->multiChannelOutput() == enabled) {
                return;
            }
            syncingFromModel = true;
            masterTrackViewModel->setMultiChannelOutput(enabled);
            syncingFromModel = false;
        });

        connect(masterTrackViewModel, &sflow::TrackViewModel::muteChanged, this, [=] {
            if (!stateMachine->configuration().contains(muteEditingState)) {
                masterTrackViewModel->setMute(control->mute());
                return;
            }
        });
        connect(masterTrackViewModel, &sflow::TrackViewModel::gainChanged, this, [=] {
            if (!stateMachine->configuration().contains(gainProgressingState)) {
                masterTrackViewModel->setGain(toDecibel(control->gain()));
                return;
            }
        });
        connect(masterTrackViewModel, &sflow::TrackViewModel::panChanged, this, [=] {
            if (!stateMachine->configuration().contains(panProgressingState)) {
                masterTrackViewModel->setPan(control->pan());
                return;
            }
        });
        connect(masterTrackViewModel, &sflow::TrackViewModel::multiChannelOutputChanged, this, [=] {
            if (syncingFromModel) {
                return;
            }
            Q_EMIT multiChannelTransactionWillStart();
            QMetaObject::invokeMethod(this, [this] {
                Q_EMIT multiChannelTransactionWillFinish();
            }, Qt::QueuedConnection);
        });

        masterTrackViewModel->setName(tr("Master"));
        masterTrackViewModel->setMute(control->mute());
        masterTrackViewModel->setGain(toDecibel(control->gain()));
        masterTrackViewModel->setPan(control->pan());
        masterTrackViewModel->setMultiChannelOutput(master->multiChannelOutput());

        masterTrackListViewModel->insertItem(0, masterTrackViewModel);
    }

    sflow::TrackListInteractionController *MasterTrackViewModelContextData::createController(QObject *parent) {
        auto controller = new sflow::TrackListInteractionController(parent);
        controller->setClickSelectable(false);
        controller->setPrimarySceneInteraction(sflow::TrackListInteractionController::None);
        controller->setSecondarySceneInteraction(sflow::TrackListInteractionController::None);
        controller->setPrimarySelectInteraction(sflow::TrackListInteractionController::None);
        controller->setSecondarySelectInteraction(sflow::TrackListInteractionController::None);
        controller->setPrimaryItemInteraction(sflow::TrackListInteractionController::None);
        controller->setSecondaryItemInteraction(sflow::TrackListInteractionController::None);
        controller->setItemAction(sflow::TrackListInteractionController::EditMute | sflow::TrackListInteractionController::EditGain | sflow::TrackListInteractionController::EditPan | sflow::TrackListInteractionController::EditMultiChannelOutput);

        connect(controller, &sflow::TrackListInteractionController::muteEditingStarted, this, [=](QQuickItem *, int) {
            Q_EMIT muteTransactionWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::muteEditingFinished, this, [=](QQuickItem *, int) {
            Q_EMIT muteTransactionWillFinish();
        });

        connect(controller, &sflow::TrackListInteractionController::gainEditingStarted, this, [=](QQuickItem *, int) {
            Q_EMIT gainTransactionWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::gainEditingCommitted, this, [=](QQuickItem *, int) {
            Q_EMIT gainTransactionWillCommit();
        });
        connect(controller, &sflow::TrackListInteractionController::gainEditingAborted, this, [=](QQuickItem *, int) {
            Q_EMIT gainTransactionWillAbort();
        });

        connect(controller, &sflow::TrackListInteractionController::panEditingStarted, this, [=](QQuickItem *, int) {
            Q_EMIT panTransactionWillStart();
        });
        connect(controller, &sflow::TrackListInteractionController::panEditingCommitted, this, [=](QQuickItem *, int) {
            Q_EMIT panTransactionWillCommit();
        });
        connect(controller, &sflow::TrackListInteractionController::panEditingAborted, this, [=](QQuickItem *, int) {
            Q_EMIT panTransactionWillAbort();
        });

        return controller;
    }

    void MasterTrackViewModelContextData::onMutePendingStateEntered() {
        muteTransactionId = document->transactionController()->beginTransaction();
        if (muteTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT muteTransactionStarted();
        } else {
            Q_EMIT muteTransactionNotStarted();
        }
    }

    void MasterTrackViewModelContextData::onMuteFinishingStateEntered() {
        if (muteTransactionId == Core::TransactionController::TransactionId::Invalid) {
            return;
        }
        const bool newValue = masterTrackViewModel->isMute();
        if (newValue == master->control()->mute()) {
            document->transactionController()->abortTransaction(muteTransactionId);
        } else {
            master->control()->setMute(newValue);
            document->transactionController()->commitTransaction(muteTransactionId, tr("Editing master mute"));
        }
        muteTransactionId = {};
    }

    void MasterTrackViewModelContextData::onGainPendingStateEntered() {
        gainTransactionId = document->transactionController()->beginTransaction();
        if (gainTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT gainTransactionStarted();
        } else {
            Q_EMIT gainTransactionNotStarted();
        }
    }

    void MasterTrackViewModelContextData::onGainCommittingStateEntered() {
        if (gainTransactionId == Core::TransactionController::TransactionId::Invalid) {
            return;
        }
        const double newLinear = toLinear(masterTrackViewModel->gain());
        if (qFuzzyCompare(newLinear, master->control()->gain())) {
            document->transactionController()->abortTransaction(gainTransactionId);
        } else {
            master->control()->setGain(newLinear);
            document->transactionController()->commitTransaction(gainTransactionId, tr("Adjusting master gain"));
        }
        gainTransactionId = {};
    }

    void MasterTrackViewModelContextData::onGainAbortingStateEntered() {
        document->transactionController()->abortTransaction(gainTransactionId);
        gainTransactionId = {};
    }

    void MasterTrackViewModelContextData::onPanPendingStateEntered() {
        panTransactionId = document->transactionController()->beginTransaction();
        if (panTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT panTransactionStarted();
        } else {
            Q_EMIT panTransactionNotStarted();
        }
    }

    void MasterTrackViewModelContextData::onPanCommittingStateEntered() {
        if (panTransactionId == Core::TransactionController::TransactionId::Invalid) {
            return;
        }
        const double newPan = masterTrackViewModel->pan();
        if (qFuzzyCompare(newPan, master->control()->pan())) {
            document->transactionController()->abortTransaction(panTransactionId);
        } else {
            master->control()->setPan(newPan);
            document->transactionController()->commitTransaction(panTransactionId, tr("Adjusting master pan"));
        }
        panTransactionId = {};
    }

    void MasterTrackViewModelContextData::onPanAbortingStateEntered() {
        document->transactionController()->abortTransaction(panTransactionId);
        panTransactionId = {};
    }

    void MasterTrackViewModelContextData::onMultiChannelPendingStateEntered() {
        multiChannelTransactionId = document->transactionController()->beginTransaction();
        if (multiChannelTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT multiChannelTransactionStarted();
        } else {
            syncingFromModel = true;
            masterTrackViewModel->setMultiChannelOutput(master->multiChannelOutput());
            syncingFromModel = false;
            Q_EMIT multiChannelTransactionNotStarted();
        }
    }

    void MasterTrackViewModelContextData::onMultiChannelFinishingStateEntered() {
        if (multiChannelTransactionId == Core::TransactionController::TransactionId::Invalid) {
            return;
        }
        const bool newValue = masterTrackViewModel->multiChannelOutput();
        if (newValue == master->multiChannelOutput()) {
            document->transactionController()->abortTransaction(multiChannelTransactionId);
        } else {
            master->setMultiChannelOutput(newValue);
            document->transactionController()->commitTransaction(multiChannelTransactionId, tr("Editing master multi-channel output"));
        }
        multiChannelTransactionId = {};
    }

}
