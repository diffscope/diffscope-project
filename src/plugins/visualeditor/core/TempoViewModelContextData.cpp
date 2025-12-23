#include "TempoViewModelContextData_p.h"

#include <algorithm>
#include <iterator>

#include <QLocale>
#include <QLoggingCategory>
#include <QStateMachine>
#include <QSignalTransition>

#include <ScopicFlowCore/LabelViewModel.h>
#include <ScopicFlowCore/PointSequenceViewModel.h>
#include <ScopicFlowCore/LabelSequenceInteractionController.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/TempoSequence.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/TempoSelectionModel.h>

#include <coreplugin/DspxDocument.h>
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
        for (auto item = startDocumentItem; item && item != endDocumentItem; item = tempoSequence->nextItem(item)) {
            auto viewItem = q->getTempoViewItemFromDocumentItem(item);
            Q_ASSERT(viewItem);
            viewItems.append(viewItem);
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
        return tempoSelectionModel->currentItem();
    }

    void TempoViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        tempoSequence = q->windowHandle()->projectDocumentContext()->document()->model()->timeline()->tempos();
        tempoSelectionModel = q->windowHandle()->projectDocumentContext()->document()->selectionModel()->tempoSelectionModel();

        tempoSequenceViewModel = new sflow::PointSequenceViewModel(q);

        tempoSelectionController = new TempoSelectionController(q);

        stateMachine = new QStateMachine(q);
        idleState = new QState;
        movingState = new QState;
        rubberBandDraggingState = new QState;
        stateMachine->addState(idleState);
        stateMachine->addState(movingState);
        stateMachine->addState(rubberBandDraggingState);
        stateMachine->setInitialState(idleState);
        stateMachine->start();

        QObject::connect(movingState, &QState::exited, q, [=, this] {
            QSet<dspx::Tempo *> updatedItems;
            for (auto viewItem : transactionalUpdatedTempos) {
                auto item = tempoDocumentItemMap.value(viewItem);
                Q_ASSERT(item);
                item->setPos(viewItem->position());
                updatedItems.insert(item);
            }
            transactionalUpdatedTempos.clear();
            for (auto item : updatedItems) {
                auto overlappingItems = tempoSequence->slice(item->pos(), item->pos() + 1);
                for (auto overlappingItem : overlappingItems) {
                    if (updatedItems.contains(overlappingItem))
                        continue;
                    tempoSequence->removeItem(overlappingItem);
                    q->windowHandle()->projectDocumentContext()->document()->model()->destroyItem(overlappingItem);
                }
            }
        });
    }

    void TempoViewModelContextData::bindTempoSequenceViewModel() {
        Q_Q(ProjectViewModelContext);
        QObject::connect(tempoSequence, &dspx::TempoSequence::itemInserted, tempoSequenceViewModel, [=, this](dspx::Tempo *item) {
            bindTempoDocumentItem(item);
        });
        QObject::connect(tempoSequence, &dspx::TempoSequence::itemRemoved, tempoSequenceViewModel, [=, this](dspx::Tempo *item) {
            unbindTempoDocumentItem(item);
        });
        // For tempo sequence, item will never be inserted or removed from view
        for (auto item : tempoSequence->asRange()) {
            bindTempoDocumentItem(item);
        }
        QObject::connect(tempoSelectionModel, &dspx::TempoSelectionModel::itemSelected, q, [=, this](dspx::Tempo *item, bool selected) {
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

        QObject::connect(item, &dspx::Tempo::posChanged, viewItem, [=] {
            if (viewItem->position() == item->pos())
                return;
            qCDebug(lcTempoViewModelContextData) << "Tempo item pos updated" << item << item->pos();
            viewItem->setPosition(item->pos());
        });
        QObject::connect(item, &dspx::Tempo::valueChanged, viewItem, [=] {
            qCDebug(lcTempoViewModelContextData) << "Tempo item value updated" << item << item->value();
            viewItem->setContent(QLocale().toString(item->value(), 'f', 2));
        });
        viewItem->setPosition(item->pos());
        viewItem->setContent(QLocale().toString(item->value(), 'f', 2));

        QObject::connect(viewItem, &sflow::LabelViewModel::positionChanged, item, [=] {
            if (viewItem->position() == item->pos())
                return;
            if (item->pos() == 0) {
                viewItem->setPosition(0);
                return;
            }
            qCDebug(lcTempoViewModelContextData) << "Tempo view item pos updated" << viewItem << viewItem->position();
            if (!stateMachine->configuration().contains(movingState)) {
                qCWarning(lcTempoViewModelContextData) << "Suspicious tempo view updating: moving state not entered";
            }
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

        QObject::disconnect(item, nullptr, viewItem, nullptr);
        QObject::disconnect(viewItem, nullptr, item, nullptr);

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
        moveStartTransition->setTargetState(movingState);
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
        QObject::connect(controller, &QObject::destroyed, [=] {
            idleState->removeTransition(moveStartTransition);
            idleState->removeTransition(rubberBandDragStartTransition);
            movingState->removeTransition(moveFinishTransition);
            rubberBandDraggingState->removeTransition(rubberBandDragFinishTransition);
        });
        return controller;
    }

}
