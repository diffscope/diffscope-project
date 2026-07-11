#include "TempoSelectionController_p.h"

#include <algorithm>
#include <iterator>

#include <QLoggingCategory>
#include <QQuickWindow>

#include <ScopicFlowCore/LabelViewModel.h>

#include <dspxmodelORM/Model.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelSelectionModel/TempoSelectionModel.h>
#include <dspxmodelORM/TempoSequence.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcTempoSelectionController, "diffscope.visualeditor.temposelectionsontroller")

    TempoSelectionController::TempoSelectionController(ProjectViewModelContext *parent)
        : SelectionController(parent), q(parent) {
        tempoSequence = q->windowHandle()->projectDocumentContext()->document()->model()->tempos();
        selectionModel = q->windowHandle()->projectDocumentContext()->document()->selectionModel();
        tempoSelectionModel = q->windowHandle()->projectDocumentContext()->document()->selectionModel()->tempoSelectionModel();
        connect(tempoSelectionModel, &dspx::TempoSelectionModel::currentItemChanged, this, &SelectionController::currentItemChanged);
        connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, this, &SelectionController::editScopeFocusedChanged);
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
        if (startDocumentItem->position() > endDocumentItem->position()) {
            std::swap(startDocumentItem, endDocumentItem);
        }
        for (auto item = startDocumentItem; item; item = item->nextItem()) {
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
        qCDebug(lcTempoSelectionController) << "Tempo view item selected" << item << command;
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

    bool TempoSelectionController::editScopeFocused() const {
        return selectionModel->selectionType() == dspx::SelectionModel::ST_Tempo;
    }

}
