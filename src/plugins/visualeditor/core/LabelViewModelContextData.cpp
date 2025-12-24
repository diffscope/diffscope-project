#include "LabelViewModelContextData_p.h"
#include "LabelSelectionController_p.h"

#include <QLoggingCategory>
#include <QQuickItem>
#include <QQuickWindow>
#include <QState>
#include <QStateMachine>

#include <ScopicFlowCore/LabelSequenceInteractionController.h>
#include <ScopicFlowCore/LabelViewModel.h>
#include <ScopicFlowCore/PointSequenceViewModel.h>
#include <ScopicFlowCore/TimeManipulator.h>
#include <ScopicFlowCore/TimeViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>

#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSelectionModel.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Timeline.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcLabelViewModelContextData, "diffscope.visualeditor.labelviewmodelcontextdata")

    void LabelViewModelContextData::initStateMachine() {
        stateMachine = new QStateMachine(QState::ExclusiveStates, this);
        idleState = new QState;
        movePendingState = new QState;
        movingState = new QState;
        rubberBandDraggingState = new QState;
        editPendingState = new QState;
        editProgressingState = new QState;
        editCommittingState = new QState;
        editAbortingState = new QState;
        insertPendingState = new QState;
        insertProgressingState = new QState;
        insertCommittingState = new QState;
        insertAbortingState = new QState;
        stateMachine->addState(idleState);
        stateMachine->addState(movePendingState);
        stateMachine->addState(movingState);
        stateMachine->addState(rubberBandDraggingState);
        stateMachine->addState(editPendingState);
        stateMachine->addState(editProgressingState);
        stateMachine->addState(editCommittingState);
        stateMachine->addState(editAbortingState);
        stateMachine->addState(insertPendingState);
        stateMachine->addState(insertProgressingState);
        stateMachine->addState(insertCommittingState);
        stateMachine->addState(insertAbortingState);
        stateMachine->setInitialState(idleState);
        stateMachine->start();

        idleState->addTransition(this, &LabelViewModelContextData::moveTransactionWillStart, movePendingState);
        movePendingState->addTransition(this, &LabelViewModelContextData::moveTransactionStarted, movingState);
        movePendingState->addTransition(this, &LabelViewModelContextData::moveTransactionNotStarted, idleState);
        movingState->addTransition(this, &LabelViewModelContextData::moveTransactionWillFinish, idleState);

        idleState->addTransition(this, &LabelViewModelContextData::rubberBandDragWillStart, rubberBandDraggingState);
        rubberBandDraggingState->addTransition(this, &LabelViewModelContextData::rubberBandDragWillFinish, idleState);

        idleState->addTransition(this, &LabelViewModelContextData::editTransactionWillStart, editPendingState);
        editPendingState->addTransition(this, &LabelViewModelContextData::editTransactionStarted, editProgressingState);
        editPendingState->addTransition(this, &LabelViewModelContextData::editTransactionNotStarted, idleState);
        editProgressingState->addTransition(this, &LabelViewModelContextData::editOrInsertTransactionWillCommit, editCommittingState);
        editProgressingState->addTransition(this, &LabelViewModelContextData::editOrInsertTransactionWillAbort, editAbortingState);
        editCommittingState->addTransition(idleState);
        editAbortingState->addTransition(idleState);

        idleState->addTransition(this, &LabelViewModelContextData::insertTransactionWillStart, insertPendingState);
        insertPendingState->addTransition(this, &LabelViewModelContextData::insertTransactionStarted, insertProgressingState);
        insertPendingState->addTransition(this, &LabelViewModelContextData::insertTransactionNotStarted, idleState);
        insertProgressingState->addTransition(this, &LabelViewModelContextData::editOrInsertTransactionWillCommit, insertCommittingState);
        insertProgressingState->addTransition(this, &LabelViewModelContextData::editOrInsertTransactionWillAbort, insertAbortingState);
        insertCommittingState->addTransition(idleState);
        insertAbortingState->addTransition(idleState);

        connect(idleState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Idle state entered";
        });
        connect(idleState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Idle state exited";
        });
        connect(movePendingState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Move pending state entered";
            onMovePendingStateEntered();
        });
        connect(movePendingState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Move pending state exited";
        });
        connect(movingState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Moving state entered";
        });
        connect(movingState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Moving state exited";
            onMovingStateExited();
        });
        connect(rubberBandDraggingState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Rubber band dragging state entered";
        });
        connect(rubberBandDraggingState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Rubber band dragging state exited";
        });
        connect(editPendingState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Edit pending state entered";
            onEditPendingStateEntered();
        });
        connect(editPendingState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Edit pending state exited";
        });
        connect(editProgressingState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Edit progressing state entered";
        });
        connect(editProgressingState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Edit progressing state exited";
        });
        connect(editCommittingState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Edit commiting state entered";
            onEditCommittingStateEntered();
        });
        connect(editCommittingState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Edit commiting state exited";
        });
        connect(editAbortingState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Edit aborting state entered";
            onEditAbortingStateEntered();
        });
        connect(editAbortingState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Edit aborting state exited";
        });
        connect(insertPendingState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Insert pending state entered";
            onInsertPendingStateEntered();
        });
        connect(insertPendingState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Insert pending state exited";
        });
        connect(insertProgressingState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Insert progressing state entered";
        });
        connect(insertProgressingState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Insert progressing state exited";
        });
        connect(insertCommittingState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Insert commiting state entered";
            onInsertCommittingStateEntered();
        });
        connect(insertCommittingState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Insert commiting state exited";
        });
        connect(insertAbortingState, &QState::entered, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Insert aborting state entered";
            onInsertAbortingStateEntered();
        });
        connect(insertAbortingState, &QState::exited, this, [=, this] {
            qCInfo(lcLabelViewModelContextData) << "Insert aborting state exited";
        });
    }

    void LabelViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        document = q->windowHandle()->projectDocumentContext()->document();
        labelSequence = document->model()->timeline()->labels();
        labelSelectionModel = document->selectionModel()->labelSelectionModel();

        labelSequenceViewModel = new sflow::PointSequenceViewModel(q);
        labelSelectionController = new LabelSelectionController(q);

        initStateMachine();
    }

    void LabelViewModelContextData::bindLabelSequenceViewModel() {
        connect(labelSequence, &dspx::LabelSequence::itemInserted, labelSequenceViewModel, [=, this](dspx::Label *item) {
            bindLabelDocumentItem(item);
        });
        connect(labelSequence, &dspx::LabelSequence::itemRemoved, labelSequenceViewModel, [=, this](dspx::Label *item) {
            unbindLabelDocumentItem(item);
        });

        for (auto item : labelSequence->asRange()) {
            bindLabelDocumentItem(item);
        }

        connect(labelSelectionModel, &dspx::LabelSelectionModel::itemSelected, this, [=, this](dspx::Label *item, bool selected) {
            qCDebug(lcLabelViewModelContextData) << "Label item selected" << item << selected;
            auto viewItem = labelViewItemMap.value(item);
            Q_ASSERT(viewItem);
            viewItem->setSelected(selected);
        });
    }

    void LabelViewModelContextData::bindLabelDocumentItem(dspx::Label *item) {
        if (labelViewItemMap.contains(item)) {
            return;
        }
        auto viewItem = new sflow::LabelViewModel(labelSequenceViewModel);
        labelViewItemMap.insert(item, viewItem);
        labelDocumentItemMap.insert(viewItem, item);
        qCDebug(lcLabelViewModelContextData) << "Label item inserted" << item << viewItem << item->pos() << item->text();

        connect(item, &dspx::Label::posChanged, viewItem, [=] {
            if (viewItem->position() == item->pos()) {
                return;
            }
            qCDebug(lcLabelViewModelContextData) << "Label item pos updated" << item << item->pos();
            viewItem->setPosition(item->pos());
        });
        connect(item, &dspx::Label::textChanged, viewItem, [=] {
            qCDebug(lcLabelViewModelContextData) << "Label item text updated" << item << item->text();
            viewItem->setContent(item->text());
        });
        viewItem->setPosition(item->pos());
        viewItem->setContent(item->text());

        connect(viewItem, &sflow::LabelViewModel::positionChanged, item, [=] {
            if (viewItem->position() == item->pos()) {
                return;
            }

            if (!stateMachine->configuration().contains(movingState)) {
                viewItem->setPosition(item->pos());
                return;
            }

            qCDebug(lcLabelViewModelContextData) << "Label view item pos updated" << viewItem << viewItem->position();
            transactionalUpdatedLabels.insert(viewItem);
        });

        labelSequenceViewModel->insertItem(viewItem);
    }

    void LabelViewModelContextData::unbindLabelDocumentItem(dspx::Label *item) {
        if (!labelViewItemMap.contains(item)) {
            return;
        }
        auto viewItem = labelViewItemMap.take(item);
        labelDocumentItemMap.remove(viewItem);
        labelViewItemMap.remove(item);
        qCDebug(lcLabelViewModelContextData) << "Label item removed" << item << viewItem;

        disconnect(item, nullptr, viewItem, nullptr);
        disconnect(viewItem, nullptr, item, nullptr);

        transactionalUpdatedLabels.remove(viewItem);

        labelSequenceViewModel->removeItem(viewItem);

        viewItem->deleteLater();
    }

    sflow::LabelSequenceInteractionController *LabelViewModelContextData::createController(QObject *parent) {
        auto controller = new sflow::LabelSequenceInteractionController(parent);
        controller->setInteraction(sflow::LabelSequenceInteractionController::SelectByRubberBand);
        controller->setItemInteraction(sflow::LabelSequenceInteractionController::Move | sflow::LabelSequenceInteractionController::Select);

        connect(controller, &sflow::LabelSequenceInteractionController::interactionOperationStarted, this, [=](QQuickItem *, sflow::LabelSequenceInteractionController::InteractionFlag type) {
            if (type == sflow::LabelSequenceInteractionController::SelectByRubberBand) {
                Q_EMIT rubberBandDragWillStart();
            }
        });
        connect(controller, &sflow::LabelSequenceInteractionController::interactionOperationFinished, this, [=](QQuickItem *, sflow::LabelSequenceInteractionController::InteractionFlag type) {
            if (type == sflow::LabelSequenceInteractionController::SelectByRubberBand) {
                Q_EMIT rubberBandDragWillFinish();
            }
        });
        connect(controller, &sflow::LabelSequenceInteractionController::itemInteractionOperationStarted, this, [=](QQuickItem *, sflow::LabelViewModel *, sflow::LabelSequenceInteractionController::ItemInteractionFlag type) {
            if (type == sflow::LabelSequenceInteractionController::Move) {
                Q_EMIT moveTransactionWillStart();
            }
        });
        connect(controller, &sflow::LabelSequenceInteractionController::itemInteractionOperationFinished, this, [=](QQuickItem *, sflow::LabelViewModel *, sflow::LabelSequenceInteractionController::ItemInteractionFlag type) {
            if (type == sflow::LabelSequenceInteractionController::Move) {
                Q_EMIT moveTransactionWillFinish();
            }
        });
        connect(controller, &sflow::LabelSequenceInteractionController::inPlaceEditOperationTriggered, this, [=](QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem, sflow::LabelSequenceInteractionController::InPlaceEditOperation type) {
            switch (type) {
                case sflow::LabelSequenceInteractionController::StartEditing:
                    break;
                case sflow::LabelSequenceInteractionController::CommitEditing:
                    Q_EMIT editOrInsertTransactionWillCommit();
                    break;
                case sflow::LabelSequenceInteractionController::AbortEditing:
                    Q_EMIT editOrInsertTransactionWillAbort();
                    break;
                case sflow::LabelSequenceInteractionController::MovePrevious:
                case sflow::LabelSequenceInteractionController::MoveNext:
                case sflow::LabelSequenceInteractionController::MoveHome:
                case sflow::LabelSequenceInteractionController::MoveEnd: {
                    QMetaObject::invokeMethod(targetLabelSequenceItem, "editInPlace", Q_ARG(sflow::LabelViewModel *, nullptr));
                    auto item = labelDocumentItemMap.value(viewItem);
                    dspx::Label *newTargetDocumentItem;
                    if (type == sflow::LabelSequenceInteractionController::MovePrevious) {
                        newTargetDocumentItem = labelSequence->previousItem(item);
                    } else if (type == sflow::LabelSequenceInteractionController::MoveNext) {
                        newTargetDocumentItem = labelSequence->nextItem(item);
                    } else if (type == sflow::LabelSequenceInteractionController::MoveHome) {
                        newTargetDocumentItem = labelSequence->firstItem();
                    } else {
                        newTargetDocumentItem = labelSequence->lastItem();
                    }
                    if (!newTargetDocumentItem) {
                        break;
                    }
                    targetLabelSequenceItem = labelSequenceItem;
                    targetDocumentItem = newTargetDocumentItem;
                    Q_EMIT editTransactionWillStart();
                }
            }
        });
        connect(controller, &sflow::LabelSequenceInteractionController::doubleClicked, this, [=](QQuickItem *labelSequenceItem, int position) {
            qCDebug(lcLabelViewModelContextData) << "Label sequence double clicked" << position;
            onDoubleClicked(labelSequenceItem, position);
        });
        connect(controller, &sflow::LabelSequenceInteractionController::itemDoubleClicked, this, [=](QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem) {
            qCDebug(lcLabelViewModelContextData) << "Label sequence view item double clicked" << viewItem;
            onItemDoubleClicked(labelSequenceItem, viewItem);
        });
        return controller;
    }

    void LabelViewModelContextData::onMovePendingStateEntered() {
        moveTransactionId = document->transactionController()->beginTransaction();
        if (moveTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT moveTransactionStarted();
        } else {
            Q_EMIT moveTransactionNotStarted();
        }
    }

    void LabelViewModelContextData::onMovingStateExited() {
        for (auto viewItem : transactionalUpdatedLabels) {
            auto item = labelDocumentItemMap.value(viewItem);
            Q_ASSERT(item);
            item->setPos(viewItem->position());
        }
        transactionalUpdatedLabels.clear();
        document->transactionController()->commitTransaction(moveTransactionId, tr("Moving label"));
        moveTransactionId = {};
    }

    void LabelViewModelContextData::onEditPendingStateEntered() {
        editTransactionId = document->transactionController()->beginTransaction();
        if (editTransactionId != Core::TransactionController::TransactionId::Invalid) {
            auto viewItem = labelViewItemMap.value(targetDocumentItem);
            QMetaObject::invokeMethod(targetLabelSequenceItem, "editInPlace", viewItem);
            Q_EMIT editTransactionStarted();
        } else {
            Q_EMIT editTransactionNotStarted();
        }
    }

    void LabelViewModelContextData::onEditCommittingStateEntered() {
        auto viewItem = labelViewItemMap.value(targetDocumentItem);
        if (targetDocumentItem->text() == viewItem->content()) {
            // no change, abort
            document->transactionController()->abortTransaction(editTransactionId);
        } else {
            targetDocumentItem->setText(viewItem->content());
            document->transactionController()->commitTransaction(editTransactionId, tr("Editing label"));
        }
        editTransactionId = {};
        targetLabelSequenceItem = {};
        targetDocumentItem = {};
        targetPosition = {};
    }

    void LabelViewModelContextData::onEditAbortingStateEntered() {
        document->transactionController()->abortTransaction(editTransactionId);
        editTransactionId = {};
        targetLabelSequenceItem = {};
        targetDocumentItem = {};
        targetPosition = {};
    }

    void LabelViewModelContextData::onInsertPendingStateEntered() {
        insertTransactionId = document->transactionController()->beginTransaction();
        if (insertTransactionId != Core::TransactionController::TransactionId::Invalid) {
            targetDocumentItem = document->model()->createLabel();
            targetDocumentItem->setPos(targetPosition);
            labelSequence->insertItem(targetDocumentItem);
            auto viewItem = labelViewItemMap.value(targetDocumentItem);
            QMetaObject::invokeMethod(targetLabelSequenceItem, "editInPlace", viewItem);
            Q_EMIT insertTransactionStarted();
        } else {
            Q_EMIT insertTransactionNotStarted();
        }
    }

    void LabelViewModelContextData::onInsertCommittingStateEntered() {
        targetDocumentItem->setText(labelViewItemMap.value(targetDocumentItem)->content());
        document->transactionController()->commitTransaction(insertTransactionId, tr("Inserting label"));
        insertTransactionId = {};
        targetLabelSequenceItem = {};
        targetDocumentItem = {};
        targetPosition = {};
    }

    void LabelViewModelContextData::onInsertAbortingStateEntered() {
        document->transactionController()->abortTransaction(insertTransactionId);
        insertTransactionId = {};
        targetLabelSequenceItem = {};
        targetDocumentItem = {};
        targetPosition = {};
    }

    void LabelViewModelContextData::onDoubleClicked(QQuickItem *labelSequenceItem, int position) {
        Q_UNUSED(labelSequenceItem)
        targetLabelSequenceItem = labelSequenceItem;
        sflow::TimeManipulator timeManipulator;
        timeManipulator.setTarget(labelSequenceItem);
        timeManipulator.setTimeViewModel(labelSequenceItem->property("timeViewModel").value<sflow::TimeViewModel *>());
        timeManipulator.setTimeLayoutViewModel(labelSequenceItem->property("timeLayoutViewModel").value<sflow::TimeLayoutViewModel *>());
        targetPosition = timeManipulator.alignPosition(position, sflow::ScopicFlow::AO_Visible);
        Q_EMIT insertTransactionWillStart();
    }

    void LabelViewModelContextData::onItemDoubleClicked(QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem) {
        Q_UNUSED(labelSequenceItem)
        targetLabelSequenceItem = labelSequenceItem;
        targetDocumentItem = labelDocumentItemMap.value(viewItem);
        Q_EMIT editTransactionWillStart();
    }

}
