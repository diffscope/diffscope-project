#include "TempoViewModelContextData_p.h"

#include <algorithm>
#include <iterator>

#include <QLocale>
#include <QLoggingCategory>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalTransition>
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

    TempoSelectionController::TempoSelectionController(ProjectViewModelContext *parent) : SelectionController(parent), q(parent) {
        tempoSequence = q->windowHandle()->projectDocumentContext()->document()->model()->timeline()->tempos();
        selectionModel = q->windowHandle()->projectDocumentContext()->document()->selectionModel();
        tempoSelectionModel = q->windowHandle()->projectDocumentContext()->document()->selectionModel()->tempoSelectionModel();
        connect(tempoSelectionModel, &dspx::TempoSelectionModel::currentItemChanged, this, &SelectionController::currentItemChanged);
    }

    TempoSelectionController::~TempoSelectionController() = default;

    QObjectList TempoSelectionController::getSelectedItems() const {
        QObjectList viewItems;
        std::ranges::transform(tempoSelectionModel->selectedItems(), std::back_inserter(viewItems), [=, this](dspx::Tempo *item) {
            auto viewItem = q->getTempoViewItemFromDocumentItem(item);
            Q_ASSERT(viewItem);
            return viewItem;
        });
        return viewItems;
    }
    QObjectList TempoSelectionController::getItemsBetween(QObject *startItem, QObject *endItem) const {
        auto startDocumentItem = startItem ? q->getTempoDocumentItemFromViewItem(qobject_cast<sflow::LabelViewModel *>(startItem)) : tempoSequence->firstItem();
        auto endDocumentItem = endItem ? q->getTempoDocumentItemFromViewItem(qobject_cast<sflow::LabelViewModel *>(endItem)) : tempoSequence->lastItem();
        Q_ASSERT(startDocumentItem && endDocumentItem);
        QObjectList viewItems;
        if (startDocumentItem->pos() > endDocumentItem->pos()) {
            std::swap(startDocumentItem, endDocumentItem);
        }
        for (auto item = startDocumentItem; item; item = tempoSequence->nextItem(item)) {
            auto viewItem = q->getTempoViewItemFromDocumentItem(item);
            Q_ASSERT(viewItem);
            viewItems.append(viewItem);
            if (item == endDocumentItem) {
                break;
            }
        }
        return viewItems;
    }
    void TempoSelectionController::select(QObject *item, SelectionCommand command) {
        qCDebug(lcTempoViewModelContextData) << "Tempo view item selected" << item << command;
        dspx::SelectionModel::SelectionCommand documentSelectionCommand = {};
        if (command & Select) {
            documentSelectionCommand |= dspx::SelectionModel::Select;
        }
        if (command & Deselect) {
            documentSelectionCommand |= dspx::SelectionModel::Deselect;
        }
        if (command & ClearPreviousSelection) {
            documentSelectionCommand |= dspx::SelectionModel::ClearPreviousSelection;
        }
        if (command & SetCurrentItem) {
            documentSelectionCommand |= dspx::SelectionModel::SetCurrentItem;
        }
        auto documentItem = q->getTempoDocumentItemFromViewItem(qobject_cast<sflow::LabelViewModel *>(item));
        Q_ASSERT(!item || documentItem);
        selectionModel->select(documentItem, documentSelectionCommand, dspx::SelectionModel::ST_Tempo);
    }
    QObject *TempoSelectionController::currentItem() const {
        return q->getTempoViewItemFromDocumentItem(tempoSelectionModel->currentItem());
    }

    void TempoViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        tempoSequence = q->windowHandle()->projectDocumentContext()->document()->model()->timeline()->tempos();
        tempoSelectionModel = q->windowHandle()->projectDocumentContext()->document()->selectionModel()->tempoSelectionModel();

        tempoSequenceViewModel = new sflow::PointSequenceViewModel(q);

        tempoSelectionController = new TempoSelectionController(q);

        stateMachine = new QStateMachine(QState::ExclusiveStates, q);
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

        movePendingState->addTransition(this, &TempoViewModelContextData::transactionStarted, movingState);
        movePendingState->addTransition(this, &TempoViewModelContextData::transactionNotStarted, idleState);

        connect(idleState, &QState::entered, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Idle state entered";
        });
        connect(idleState, &QState::exited, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Idle state exited";
        });
        connect(movePendingState, &QState::entered, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Move pending state entered";
            handleMovePendingStateEntered();
        });
        connect(movePendingState, &QState::exited, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Move pending state exited";
        });
        connect(movingState, &QState::entered, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Moving state entered";
        });
        connect(movingState, &QState::exited, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Moving state exited";
            handleMovingStateExited();
        });
        connect(rubberBandDraggingState, &QState::entered, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Rubber band dragging state entered";
        });
        connect(rubberBandDraggingState, &QState::exited, this, [=, this] {
            qCInfo(lcTempoViewModelContextData) << "Rubber band dragging state exited";
        });

        scenario = new Core::EditTempoTimeSignatureScenario(this);
        scenario->setProjectTimeline(q->windowHandle()->projectTimeline());
        scenario->setDocument(q->windowHandle()->projectDocumentContext()->document());
        scenario->setShouldDialogPopupAtCursor(true);
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
            if (viewItem->position() == item->pos())
                return;
            if (item->pos() == 0) {
                viewItem->setPosition(item->pos());
                return;
            }
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

    class InteractionSignalTransition : public QSignalTransition {
    public:
        template <typename PointerToMemberFunction>
        explicit InteractionSignalTransition(sflow::LabelSequenceInteractionController *controller, sflow::LabelSequenceInteractionController::InteractionFlag flag, PointerToMemberFunction signal)
            : QSignalTransition(controller, signal), m_flag(flag) {
        }

    protected:
        bool eventTest(QEvent *event) override {
            if (!QSignalTransition::eventTest(event))
                return false;
            auto se = static_cast<QStateMachine::SignalEvent*>(event);
            return se->arguments().at(1).toInt() == m_flag;
        }

    private:
        sflow::LabelSequenceInteractionController::InteractionFlag m_flag;
    };

    class ItemInteractionSignalTransition : public QSignalTransition {
    public:
        template <typename PointerToMemberFunction>
        explicit ItemInteractionSignalTransition(sflow::LabelSequenceInteractionController *controller, sflow::LabelSequenceInteractionController::ItemInteractionFlag flag, PointerToMemberFunction signal)
            : QSignalTransition(controller, signal), m_flag(flag) {
        }

    protected:
        bool eventTest(QEvent *event) override {
            if (!QSignalTransition::eventTest(event))
                return false;
            auto se = static_cast<QStateMachine::SignalEvent*>(event);
            return se->arguments().at(2).toInt() == m_flag;
        }

    private:
        sflow::LabelSequenceInteractionController::ItemInteractionFlag m_flag;
    };

    sflow::LabelSequenceInteractionController *TempoViewModelContextData::createController(QObject *parent) {
        auto controller = new sflow::LabelSequenceInteractionController(parent);
        controller->setInteraction(sflow::LabelSequenceInteractionController::SelectByRubberBand);
        controller->setItemInteraction(sflow::LabelSequenceInteractionController::Move | sflow::LabelSequenceInteractionController::Select);
        auto moveStartTransition = new ItemInteractionSignalTransition(controller, sflow::LabelSequenceInteractionController::Move, &sflow::LabelSequenceInteractionController::itemInteractionOperationStarted);
        moveStartTransition->setTargetState(movePendingState);
        idleState->addTransition(moveStartTransition);
        auto moveFinishTransition = new ItemInteractionSignalTransition(controller, sflow::LabelSequenceInteractionController::Move, &sflow::LabelSequenceInteractionController::itemInteractionOperationFinished);
        moveFinishTransition->setTargetState(idleState);
        movingState->addTransition(moveFinishTransition);
        auto rubberBandDragStartTransition = new InteractionSignalTransition(controller, sflow::LabelSequenceInteractionController::SelectByRubberBand, &sflow::LabelSequenceInteractionController::interactionOperationStarted);
        rubberBandDragStartTransition->setTargetState(rubberBandDraggingState);
        idleState->addTransition(rubberBandDragStartTransition);
        auto rubberBandDragFinishTransition = new InteractionSignalTransition(controller, sflow::LabelSequenceInteractionController::SelectByRubberBand, &sflow::LabelSequenceInteractionController::interactionOperationFinished);
        rubberBandDragFinishTransition->setTargetState(idleState);
        rubberBandDraggingState->addTransition(rubberBandDragFinishTransition);
        connect(controller, &QObject::destroyed, [=] {
            idleState->removeTransition(moveStartTransition);
            idleState->removeTransition(rubberBandDragStartTransition);
            movingState->removeTransition(moveFinishTransition);
            rubberBandDraggingState->removeTransition(rubberBandDragFinishTransition);
        });
        connect(controller, &sflow::LabelSequenceInteractionController::doubleClicked, this, [=](QQuickItem *labelSequenceItem, int position) {
            qCDebug(lcTempoViewModelContextData) << "Tempo sequence double clicked" << position;
            handleDoubleClicked(labelSequenceItem, position);
        });
        connect(controller, &sflow::LabelSequenceInteractionController::itemDoubleClicked, this, [=](QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem) {
            qCDebug(lcTempoViewModelContextData) << "Tempo sequence view item double clicked" << viewItem;
            handleItemDoubleClicked(labelSequenceItem, viewItem);
        });
        return controller;
    }

    void TempoViewModelContextData::handleMovePendingStateEntered() {
        Q_Q(ProjectViewModelContext);
        for (auto item : tempoSelectionModel->selectedItems()) {
            if (item->pos() == 0) {
                // Not allowed to move tempo to position 0
                Q_EMIT transactionNotStarted();
                return;
            }
        }
        moveTransactionId = q->windowHandle()->projectDocumentContext()->document()->transactionController()->beginTransaction();
        if (moveTransactionId != Core::TransactionController::TransactionId::Invalid) {
            Q_EMIT transactionStarted();
        } else {
            Q_EMIT transactionNotStarted();
        }
    }

    void TempoViewModelContextData::handleMovingStateExited() {
        Q_Q(ProjectViewModelContext);
        QSet<dspx::Tempo *> updatedItems;
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
                q->windowHandle()->projectDocumentContext()->document()->model()->destroyItem(overlappingItem);
            }
        }
        q->windowHandle()->projectDocumentContext()->document()->transactionController()->commitTransaction(moveTransactionId, tr("Moving tempo"));
    }
    void TempoViewModelContextData::handleDoubleClicked(QQuickItem *labelSequenceItem, int position) {
        Q_Q(ProjectViewModelContext);
        sflow::TimeManipulator timeManipulator;
        timeManipulator.setTarget(labelSequenceItem);
        timeManipulator.setTimeViewModel(labelSequenceItem->property("timeViewModel").value<sflow::TimeViewModel *>());
        timeManipulator.setTimeLayoutViewModel(labelSequenceItem->property("timeLayoutViewModel").value<sflow::TimeLayoutViewModel *>());
        position = timeManipulator.alignPosition(position, sflow::ScopicFlow::AO_Visible);
        scenario->setWindow(labelSequenceItem->window());
        scenario->insertTempoAt(position);
        auto items = tempoSequence->slice(position, 1);
        if (items.isEmpty()) {
            return;
        }
        q->windowHandle()->projectDocumentContext()->document()->selectionModel()->select(items.first(), dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
    }
    void TempoViewModelContextData::handleItemDoubleClicked(QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem) {
        Q_Q(ProjectViewModelContext);
        scenario->setWindow(labelSequenceItem->window());
        scenario->modifyExistingTempoAt(viewItem->position());
    }

}
