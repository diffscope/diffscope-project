#include "TempoViewModelContextData_p.h"
#include "TempoSelectionController_p.h"

#include <QLocale>
#include <QLoggingCategory>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStateMachine>

#include <ScopicFlowCore/LabelSequenceInteractionController.h>
#include <ScopicFlowCore/LabelViewModel.h>
#include <ScopicFlowCore/PointSequenceViewModel.h>
#include <ScopicFlowCore/TimeManipulator.h>
#include <ScopicFlowCore/TimeViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/TempoSelectionModel.h>
#include <dspxmodel/TempoSequence.h>
#include <dspxmodel/Timeline.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/EditTempoTimeSignatureScenario.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcTempoViewModelContextData, "diffscope.visualeditor.tempoviewmodelcontextdata")

    void TempoViewModelContextData::initStateMachine() {
        stateMachine = new QStateMachine(QState::ExclusiveStates, this);
        idleState = new QState;
        movePendingState = new QState;
        movingState = new QState;
        rubberBandDraggingState = new QState;
        stateMachine->addState(idleState);
        stateMachine->addState(movePendingState);
        stateMachine->addState(movingState);
        stateMachine->addState(rubberBandDraggingState);
        stateMachine->setInitialState(idleState);
        stateMachine->start();

        idleState->addTransition(this, &TempoViewModelContextData::moveTransactionWillStart, movePendingState);
        movePendingState->addTransition(this, &TempoViewModelContextData::moveTransactionStarted, movingState);
        movePendingState->addTransition(this, &TempoViewModelContextData::moveTransactionNotStarted, idleState);
        movingState->addTransition(this, &TempoViewModelContextData::moveTransactionWillFinish, idleState);

        idleState->addTransition(this, &TempoViewModelContextData::rubberBandDragWillStart, rubberBandDraggingState);
        rubberBandDraggingState->addTransition(this, &TempoViewModelContextData::rubberBandDragWillFinish, idleState);

        connect(idleState, &QState::entered, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Idle state entered";
        });
        connect(idleState, &QState::exited, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Idle state exited";
        });
        connect(movePendingState, &QState::entered, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Move pending state entered";
            onMovePendingStateEntered();
        });
        connect(movePendingState, &QState::exited, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Move pending state exited";
        });
        connect(movingState, &QState::entered, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Moving state entered";
        });
        connect(movingState, &QState::exited, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Moving state exited";
            onMovingStateExited();
        });
        connect(rubberBandDraggingState, &QState::entered, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Rubber band dragging state entered";
        });
        connect(rubberBandDraggingState, &QState::exited, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Rubber band dragging state exited";
        });
    }
    void TempoViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        document = q->windowHandle()->projectDocumentContext()->document();
        tempoSequence = document->model()->timeline()->tempos();
        tempoSelectionModel = document->selectionModel()->tempoSelectionModel();

        tempoSequenceViewModel = new sflow::PointSequenceViewModel(q);
        tempoSelectionController = new TempoSelectionController(q);

        initStateMachine();
    }

    void TempoViewModelContextData::bindTempoSequenceViewModel() {
        Q_Q(ProjectViewModelContext);
        connect(tempoSequence, &dspx::TempoSequence::itemInserted, tempoSequenceViewModel, [=, this](dspx::Tempo *item) {
            bindTempoDocumentItem(item);
        });
        connect(tempoSequence, &dspx::TempoSequence::itemRemoved, tempoSequenceViewModel, [=, this](dspx::Tempo *item) {
            unbindTempoDocumentItem(item);
        });
        // For tempo sequence, item will never be inserted or removed from view
        for (auto item : tempoSequence->asRange()) {
            bindTempoDocumentItem(item);
        }
        connect(tempoSelectionModel, &dspx::TempoSelectionModel::itemSelected, this, [=, this](dspx::Tempo *item, bool selected) {
            qCDebug(lcTempoViewModelContextData) << "Tempo item selected" << item << selected;
            auto viewItem = tempoViewItemMap.value(item);
            Q_ASSERT(viewItem);
            viewItem->setSelected(selected);
        });
    }

    void TempoViewModelContextData::bindTempoDocumentItem(dspx::Tempo *item) {
        if (tempoViewItemMap.contains(item)) {
            return;
        }
        auto viewItem = new sflow::LabelViewModel(tempoSequenceViewModel);
        tempoViewItemMap.insert(item, viewItem);
        tempoDocumentItemMap.insert(viewItem, item);
        qCDebug(lcTempoViewModelContextData) << "Tempo item inserted" << item << viewItem << item->pos() << item->value();

        connect(item, &dspx::Tempo::posChanged, viewItem, [=] {
            if (viewItem->position() == item->pos())
                return;
            qCDebug(lcTempoViewModelContextData) << "Tempo item pos updated" << item << item->pos();
            viewItem->setPosition(item->pos());
        });
        connect(item, &dspx::Tempo::valueChanged, viewItem, [=] {
            qCDebug(lcTempoViewModelContextData) << "Tempo item value updated" << item << item->value();
            viewItem->setContent(QLocale().toString(item->value()));
        });
        viewItem->setPosition(item->pos());
        viewItem->setContent(QLocale().toString(item->value()));

        connect(viewItem, &sflow::LabelViewModel::positionChanged, item, [=] {
            if (viewItem->position() == item->pos()) {
                return;
            }

            // If the tempo is at the start of the timeline, restore the original position
            if (item->pos() == 0) {
                viewItem->setPosition(item->pos());
                return;
            }

            // If the tempo is not being moved, restore the original position
            if (!stateMachine->configuration().contains(movingState)) {
                viewItem->setPosition(item->pos());
                return;
            }

            qCDebug(lcTempoViewModelContextData) << "Tempo view item pos updated" << viewItem << viewItem->position();
            transactionalUpdatedTempos.insert(viewItem);
        });

        tempoSequenceViewModel->insertItem(viewItem);
    }

    void TempoViewModelContextData::unbindTempoDocumentItem(dspx::Tempo *item) {
        if (!tempoViewItemMap.contains(item)) {
            return;
        }
        auto viewItem = tempoViewItemMap.take(item);
        tempoDocumentItemMap.remove(viewItem);
        tempoViewItemMap.remove(item);
        qCDebug(lcTempoViewModelContextData) << "Tempo item removed" << item << viewItem;

        disconnect(item, nullptr, viewItem, nullptr);
        disconnect(viewItem, nullptr, item, nullptr);

        transactionalUpdatedTempos.remove(viewItem);

        tempoSequenceViewModel->removeItem(viewItem);

        viewItem->deleteLater();
    }

    sflow::LabelSequenceInteractionController *TempoViewModelContextData::createController(QObject *parent) {
        auto controller = new sflow::LabelSequenceInteractionController(parent);
        controller->setInteraction(sflow::LabelSequenceInteractionController::SelectByRubberBand);
        controller->setItemInteraction(sflow::LabelSequenceInteractionController::Move | sflow::LabelSequenceInteractionController::Select);

        connect(controller, &sflow::LabelSequenceInteractionController::rubberBandDraggingStarted, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillStart();
        });
        connect(controller, &sflow::LabelSequenceInteractionController::rubberBandDraggingFinished, this, [=](QQuickItem *) {
            Q_EMIT rubberBandDragWillFinish();
        });
        connect(controller, &sflow::LabelSequenceInteractionController::movingStarted, this, [=](QQuickItem *, sflow::LabelViewModel *) {
            Q_EMIT moveTransactionWillStart();
        });
        connect(controller, &sflow::LabelSequenceInteractionController::movingFinished, this, [=](QQuickItem *, sflow::LabelViewModel *) {
            Q_EMIT moveTransactionWillFinish();
        });

        connect(controller, &sflow::LabelSequenceInteractionController::doubleClicked, this, [=](QQuickItem *labelSequenceItem, int position) {
            qCDebug(lcTempoViewModelContextData) << "Tempo sequence double clicked" << position;
            onDoubleClicked(labelSequenceItem, position);
        });
        connect(controller, &sflow::LabelSequenceInteractionController::itemDoubleClicked, this, [=](QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem) {
            qCDebug(lcTempoViewModelContextData) << "Tempo sequence view item double clicked" << viewItem;
            onItemDoubleClicked(labelSequenceItem, viewItem);
        });
        return controller;
    }

    void TempoViewModelContextData::onMovePendingStateEntered() {
        Q_Q(ProjectViewModelContext);
        for (auto item : tempoSelectionModel->selectedItems()) {
            if (item->pos() == 0) {
                // Not allowed to move tempo to position 0
                Q_EMIT moveTransactionNotStarted();
                return;
            }
        }
        moveTransactionId = document->transactionController()->beginTransaction();
        if (moveTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT moveTransactionStarted();
        } else {
            Q_EMIT moveTransactionNotStarted();
        }
    }

    void TempoViewModelContextData::onMovingStateExited() {
        Q_Q(ProjectViewModelContext);
        QSet<dspx::Tempo *> updatedItems;
        if (transactionalUpdatedTempos.isEmpty()) {
            document->transactionController()->abortTransaction(moveTransactionId);
            moveTransactionId = {};
            return;
        }
        for (auto viewItem : transactionalUpdatedTempos) {
            auto item = tempoDocumentItemMap.value(viewItem);
            Q_ASSERT(item);
            item->setPos(viewItem->position());
            updatedItems.insert(item);
        }
        transactionalUpdatedTempos.clear();
        for (auto item : updatedItems) {
            auto overlappingItems = tempoSequence->slice(item->pos(), 1);
            for (auto overlappingItem : overlappingItems) {
                if (updatedItems.contains(overlappingItem))
                    continue;
                tempoSequence->removeItem(overlappingItem);
                document->model()->destroyItem(overlappingItem);
            }
        }
        document->transactionController()->commitTransaction(moveTransactionId, tr("Moving tempo"));
        moveTransactionId = {};
    }

    void TempoViewModelContextData::onDoubleClicked(QQuickItem *labelSequenceItem, int position) {
        Q_Q(ProjectViewModelContext);
        sflow::TimeManipulator timeManipulator;
        timeManipulator.setTarget(labelSequenceItem);
        timeManipulator.setTimeViewModel(labelSequenceItem->property("timeViewModel").value<sflow::TimeViewModel *>());
        timeManipulator.setTimeLayoutViewModel(labelSequenceItem->property("timeLayoutViewModel").value<sflow::TimeLayoutViewModel *>());
        position = timeManipulator.alignPosition(position, sflow::ScopicFlow::AO_Visible);
        Core::EditTempoTimeSignatureScenario scenario;
        scenario.setProjectTimeline(q->windowHandle()->projectTimeline());
        scenario.setDocument(document);
        scenario.setShouldDialogPopupAtCursor(true);
        scenario.setWindow(labelSequenceItem->window());
        scenario.insertTempoAt(position);
        auto items = tempoSequence->slice(position, 1);
        if (items.isEmpty()) {
            return;
        }
        document->selectionModel()->select(items.first(), dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
    }

    void TempoViewModelContextData::onItemDoubleClicked(QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem) {
        Q_Q(ProjectViewModelContext);
        Core::EditTempoTimeSignatureScenario scenario;
        scenario.setProjectTimeline(q->windowHandle()->projectTimeline());
        scenario.setDocument(document);
        scenario.setShouldDialogPopupAtCursor(true);
        scenario.setWindow(labelSequenceItem->window());
        scenario.modifyExistingTempoAt(viewItem->position());
    }

}
