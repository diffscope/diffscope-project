#include "KeySignatureViewModelContextData_p.h"

#include <QLocale>
#include <QLoggingCategory>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStateMachine>

#include <SVSCraftCore/MusicPitch.h>
#include <SVSCraftCore/MusicMode.h>
#include <SVSCraftCore/MusicModeInfo.h>

#include <ScopicFlowCore/LabelSequenceInteractionController.h>
#include <ScopicFlowCore/LabelViewModel.h>
#include <ScopicFlowCore/PointSequenceViewModel.h>
#include <ScopicFlowCore/TimeManipulator.h>
#include <ScopicFlowCore/TimeViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/KeySignature.h>
#include <dspxmodel/KeySignatureSelectionModel.h>
#include <dspxmodel/KeySignatureSequence.h>
#include <dspxmodel/Timeline.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/EditKeySignatureScenario.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>
#include <visualeditor/private/KeySignatureSelectionController_p.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcKeySignatureViewModelContextData, "diffscope.visualeditor.keysignatureviewmodelcontextdata")

    static QString getLabelText(int mode, int tonality, int accidentalType) {
        static auto map = [] {
            QMap<int, QString> map;
            for (const auto &[musicMode, name] : SVS::MusicModeInfo::getBuiltInMusicModeInfoList()) {
                map.insert(musicMode.mask(), name);
            }
            return map;
        }();
        auto modeName = map.value(mode);
        if (modeName.isEmpty()) {
            modeName = VisualEditor::KeySignatureViewModelContextData::tr("Custom Mode");
        }
        if (mode == 0) { // Atonal
            return modeName;
        }
        auto keyName = SVS::MusicPitch(tonality).toString(static_cast<SVS::MusicPitch::Accidental>(accidentalType));
        keyName = keyName.slice(0, keyName.length() - 1);
        return QString("%1 %2").arg(keyName, modeName);
    }

    void KeySignatureViewModelContextData::initStateMachine() {
        stateMachine = new QStateMachine(QState::ExclusiveStates, this);
        idleState = new QState;
        movePendingState = new QState;
        moveProgressingState = new QState;
        moveCommittingState = new QState;
        moveAbortingState = new QState;
        rubberBandDraggingState = new QState;
        stateMachine->addState(idleState);
        stateMachine->addState(movePendingState);
        stateMachine->addState(moveProgressingState);
        stateMachine->addState(moveCommittingState);
        stateMachine->addState(moveAbortingState);
        stateMachine->addState(rubberBandDraggingState);
        stateMachine->setInitialState(idleState);
        stateMachine->start();

        idleState->addTransition(this, &KeySignatureViewModelContextData::moveTransactionWillStart, movePendingState);
        movePendingState->addTransition(this, &KeySignatureViewModelContextData::moveTransactionStarted, moveProgressingState);
        movePendingState->addTransition(this, &KeySignatureViewModelContextData::moveTransactionNotStarted, idleState);
        moveProgressingState->addTransition(this, &KeySignatureViewModelContextData::moveTransactionWillCommit, moveCommittingState);
        moveProgressingState->addTransition(this, &KeySignatureViewModelContextData::moveTransactionWillAbort, moveAbortingState);
        moveCommittingState->addTransition(idleState);
        moveAbortingState->addTransition(idleState);

        idleState->addTransition(this, &KeySignatureViewModelContextData::rubberBandDragWillStart, rubberBandDraggingState);
        rubberBandDraggingState->addTransition(this, &KeySignatureViewModelContextData::rubberBandDragWillFinish, idleState);

        connect(idleState, &QState::entered, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Idle state entered";
        });
        connect(idleState, &QState::exited, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Idle state exited";
        });
        connect(movePendingState, &QState::entered, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Move pending state entered";
            onMovePendingStateEntered();
        });
        connect(movePendingState, &QState::exited, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Move pending state exited";
        });
        connect(moveProgressingState, &QState::entered, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Move progressing state entered";
        });
        connect(moveProgressingState, &QState::exited, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Move progressing state exited";
        });
        connect(moveCommittingState, &QState::entered, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Move committing state entered";
            onMoveCommittingStateEntered();
        });
        connect(moveCommittingState, &QState::exited, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Move committing state exited";
        });
        connect(moveAbortingState, &QState::entered, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Move aborting state entered";
            onMoveAbortingStateEntered();
        });
        connect(moveAbortingState, &QState::exited, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Move aborting state exited";
        });
        connect(rubberBandDraggingState, &QState::entered, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Rubber band dragging state entered";
        });
        connect(rubberBandDraggingState, &QState::exited, this, [=, this] {
            qCInfo(lcKeySignatureViewModelContextData) << "Rubber band dragging state exited";
        });
    }
    void KeySignatureViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        document = q->windowHandle()->projectDocumentContext()->document();
        keySignatureSequence = document->model()->timeline()->keySignatures();
        keySignatureSelectionModel = document->selectionModel()->keySignatureSelectionModel();

        keySignatureSequenceViewModel = new sflow::PointSequenceViewModel(q);
        keySignatureSelectionController = new KeySignatureSelectionController(q);

        initStateMachine();
    }

    void KeySignatureViewModelContextData::bindKeySignatureSequenceViewModel() {
        Q_Q(ProjectViewModelContext);
        connect(keySignatureSequence, &dspx::KeySignatureSequence::itemInserted, keySignatureSequenceViewModel, [=, this](dspx::KeySignature *item) {
            bindKeySignatureDocumentItem(item);
        });
        connect(keySignatureSequence, &dspx::KeySignatureSequence::itemRemoved, keySignatureSequenceViewModel, [=, this](dspx::KeySignature *item) {
            unbindKeySignatureDocumentItem(item);
        });
        // For key signature sequence, item will never be inserted or removed from view
        for (auto item : keySignatureSequence->asRange()) {
            bindKeySignatureDocumentItem(item);
        }
        connect(keySignatureSelectionModel, &dspx::KeySignatureSelectionModel::itemSelected, this, [=, this](dspx::KeySignature *item, bool selected) {
            qCDebug(lcKeySignatureViewModelContextData) << "KeySignature item selected" << item << selected;
            auto viewItem = keySignatureViewItemMap.value(item);
            Q_ASSERT(viewItem);
            viewItem->setSelected(selected);
        });
    }

    void KeySignatureViewModelContextData::bindKeySignatureDocumentItem(dspx::KeySignature *item) {
        if (keySignatureViewItemMap.contains(item)) {
            return;
        }
        auto viewItem = new sflow::LabelViewModel(keySignatureSequenceViewModel);
        keySignatureViewItemMap.insert(item, viewItem);
        keySignatureDocumentItemMap.insert(viewItem, item);
        qCDebug(lcKeySignatureViewModelContextData) << "KeySignature item inserted" << item << viewItem << item->pos() << item->mode() << item->tonality();

        connect(item, &dspx::KeySignature::posChanged, viewItem, [=] {
            if (viewItem->position() == item->pos())
                return;
            qCDebug(lcKeySignatureViewModelContextData) << "KeySignature item pos updated" << item << item->pos();
            viewItem->setPosition(item->pos());
        });
        connect(item, &dspx::KeySignature::modeChanged, viewItem, [=] {
            qCDebug(lcKeySignatureViewModelContextData) << "KeySignature item mode updated" << item << item->mode();
            viewItem->setContent(getLabelText(item->mode(), item->tonality(), item->accidentalType()));
        });
        connect(item, &dspx::KeySignature::tonalityChanged, viewItem, [=] {
            qCDebug(lcKeySignatureViewModelContextData) << "KeySignature item tonality updated" << item << item->tonality();
            viewItem->setContent(getLabelText(item->mode(), item->tonality(), item->accidentalType()));
        });
        connect(item, &dspx::KeySignature::accidentalTypeChanged, viewItem, [=] {
            qCDebug(lcKeySignatureViewModelContextData) << "KeySignature item accidental type updated" << item << item->accidentalType();
            viewItem->setContent(getLabelText(item->mode(), item->tonality(), item->accidentalType()));
        });
        viewItem->setPosition(item->pos());
        viewItem->setContent(getLabelText(item->mode(), item->tonality(), item->accidentalType()));

        connect(viewItem, &sflow::LabelViewModel::positionChanged, item, [=] {
            if (viewItem->position() == item->pos()) {
                return;
            }

            // Unlike tempo, KeySignature at position 0 CAN be moved and deleted
            // If the key signature is not being moved, restore the original position
            if (!stateMachine->configuration().contains(moveProgressingState)) {
                viewItem->setPosition(item->pos());
                return;
            }

            qCDebug(lcKeySignatureViewModelContextData) << "KeySignature view item pos updated" << viewItem << viewItem->position();
            transactionalUpdatedKeySignatures.insert(viewItem);
        });

        keySignatureSequenceViewModel->insertItem(viewItem);
    }

    void KeySignatureViewModelContextData::unbindKeySignatureDocumentItem(dspx::KeySignature *item) {
        if (!keySignatureViewItemMap.contains(item)) {
            return;
        }
        auto viewItem = keySignatureViewItemMap.take(item);
        keySignatureDocumentItemMap.remove(viewItem);
        keySignatureViewItemMap.remove(item);
        qCDebug(lcKeySignatureViewModelContextData) << "KeySignature item removed" << item << viewItem;

        disconnect(item, nullptr, viewItem, nullptr);
        disconnect(viewItem, nullptr, item, nullptr);

        transactionalUpdatedKeySignatures.remove(viewItem);

        keySignatureSequenceViewModel->removeItem(viewItem);

        viewItem->deleteLater();
    }

    sflow::LabelSequenceInteractionController *KeySignatureViewModelContextData::createController(QObject *parent) {
        auto controller = new sflow::LabelSequenceInteractionController(parent);
        controller->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
        controller->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
        controller->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
        controller->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
        controller->setPrimarySelectInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
        controller->setSecondarySelectInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

        connect(controller, &sflow::LabelSequenceInteractionController::rubberBandDraggingStarted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillStart();
        });
        connect(controller, &sflow::LabelSequenceInteractionController::rubberBandDraggingCommitted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillFinish();
        });
        connect(controller, &sflow::LabelSequenceInteractionController::rubberBandDraggingAborted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillFinish();
        });
        connect(controller, &sflow::LabelSequenceInteractionController::movingStarted, this, [=](QQuickItem *, sflow::LabelViewModel *) {
            Q_EMIT moveTransactionWillStart();
        });
        connect(controller, &sflow::LabelSequenceInteractionController::movingCommitted, this, [=](QQuickItem *, sflow::LabelViewModel *) {
            Q_EMIT moveTransactionWillCommit();
        });
        connect(controller, &sflow::LabelSequenceInteractionController::movingAborted, this, [=](QQuickItem *, sflow::LabelViewModel *) {
            Q_EMIT moveTransactionWillAbort();
        });

        connect(controller, &sflow::LabelSequenceInteractionController::doubleClicked, this, [=](QQuickItem *labelSequenceItem, int position) {
            qCDebug(lcKeySignatureViewModelContextData) << "KeySignature sequence double clicked" << position;
            onDoubleClicked(labelSequenceItem, position);
        });
        connect(controller, &sflow::LabelSequenceInteractionController::itemDoubleClicked, this, [=](QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem) {
            qCDebug(lcKeySignatureViewModelContextData) << "KeySignature sequence view item double clicked" << viewItem;
            onItemDoubleClicked(labelSequenceItem, viewItem);
        });
        return controller;
    }

    void KeySignatureViewModelContextData::onMovePendingStateEntered() {
        Q_Q(ProjectViewModelContext);
        // Unlike tempo, KeySignature at position 0 CAN be moved
        moveTransactionId = document->transactionController()->beginTransaction();
        if (moveTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT moveTransactionStarted();
        } else {
            Q_EMIT moveTransactionNotStarted();
        }
    }

    void KeySignatureViewModelContextData::onMoveCommittingStateEntered() {
        Q_Q(ProjectViewModelContext);
        QSet<dspx::KeySignature *> updatedItems;
        if (transactionalUpdatedKeySignatures.isEmpty()) {
            document->transactionController()->abortTransaction(moveTransactionId);
            moveTransactionId = {};
            return;
        }
        for (auto viewItem : transactionalUpdatedKeySignatures) {
            auto item = keySignatureDocumentItemMap.value(viewItem);
            Q_ASSERT(item);
            item->setPos(viewItem->position());
            updatedItems.insert(item);
        }
        transactionalUpdatedKeySignatures.clear();
        for (auto item : updatedItems) {
            auto overlappingItems = keySignatureSequence->slice(item->pos(), 1);
            for (auto overlappingItem : overlappingItems) {
                if (updatedItems.contains(overlappingItem))
                    continue;
                keySignatureSequence->removeItem(overlappingItem);
                document->model()->destroyItem(overlappingItem);
            }
        }
        document->transactionController()->commitTransaction(moveTransactionId, tr("Moving key signature"));
        moveTransactionId = {};
    }

    void KeySignatureViewModelContextData::onMoveAbortingStateEntered() {
        transactionalUpdatedKeySignatures.clear();
        document->transactionController()->abortTransaction(moveTransactionId);
        moveTransactionId = {};
    }

    void KeySignatureViewModelContextData::onDoubleClicked(QQuickItem *labelSequenceItem, int position) {
        Q_Q(ProjectViewModelContext);
        sflow::TimeManipulator timeManipulator;
        timeManipulator.setTarget(labelSequenceItem);
        timeManipulator.setTimeViewModel(labelSequenceItem->property("timeViewModel").value<sflow::TimeViewModel *>());
        timeManipulator.setTimeLayoutViewModel(labelSequenceItem->property("timeLayoutViewModel").value<sflow::TimeLayoutViewModel *>());
        position = timeManipulator.alignPosition(position, sflow::ScopicFlow::AO_Visible);
        Core::EditKeySignatureScenario scenario;
        scenario.setProjectTimeline(q->windowHandle()->projectTimeline());
        scenario.setDocument(document);
        scenario.setShouldDialogPopupAtCursor(true);
        scenario.setWindow(labelSequenceItem->window());
        scenario.insertKeySignatureAt(position);
        auto items = keySignatureSequence->slice(position, 1);
        if (items.isEmpty()) {
            return;
        }
        document->selectionModel()->select(items.first(), dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
    }

    void KeySignatureViewModelContextData::onItemDoubleClicked(QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem) {
        Q_Q(ProjectViewModelContext);
        Core::EditKeySignatureScenario scenario;
        scenario.setProjectTimeline(q->windowHandle()->projectTimeline());
        scenario.setDocument(document);
        scenario.setShouldDialogPopupAtCursor(true);
        scenario.setWindow(labelSequenceItem->window());
        scenario.modifyExistingKeySignatureAt(viewItem->position());
    }

}
