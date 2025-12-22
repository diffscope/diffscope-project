#include "TempoViewModelContextData_p.h"

#include <algorithm>
#include <iterator>

#include <QLocale>
#include <QLoggingCategory>
#include <QStateMachine>

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
        if (command | Select) {
            documentSelectionCommand |= dspx::SelectionModel::Select;
        }
        if (command | Deselect) {
            documentSelectionCommand |= dspx::SelectionModel::Deselect;
        }
        if (command | ClearPreviousSelection) {
            documentSelectionCommand |= dspx::SelectionModel::ClearPreviousSelection;
        }
        if (command | SetCurrentItem) {
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

    sflow::LabelSequenceInteractionController *TempoViewModelContextData::createController(QObject *parent) {
        // TODO
        auto controller = new sflow::LabelSequenceInteractionController(parent);
        controller->setInteraction({});
        controller->setItemInteraction({});
        return controller;
    }

}
