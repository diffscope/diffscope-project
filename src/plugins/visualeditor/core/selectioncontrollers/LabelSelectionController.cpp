#include "LabelSelectionController_p.h"

#include <algorithm>
#include <iterator>

#include <QLoggingCategory>
#include <QQuickWindow>

#include <ScopicFlowCore/LabelViewModel.h>

#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSelectionModel.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Timeline.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcLabelSelectionController, "diffscope.visualeditor.labelselectioncontroller")

    LabelSelectionController::LabelSelectionController(ProjectViewModelContext *parent)
        : SelectionController(parent), q(parent) {
        labelSequence = q->windowHandle()->projectDocumentContext()->document()->model()->timeline()->labels();
        selectionModel = q->windowHandle()->projectDocumentContext()->document()->selectionModel();
        labelSelectionModel = q->windowHandle()->projectDocumentContext()->document()->selectionModel()->labelSelectionModel();
        connect(labelSelectionModel, &dspx::LabelSelectionModel::currentItemChanged, this, &SelectionController::currentItemChanged);
        connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, this, &SelectionController::editScopeFocusedChanged);
    }

    LabelSelectionController::~LabelSelectionController() = default;

    QObjectList LabelSelectionController::getSelectedItems() const {
        QObjectList viewItems;
        std::ranges::transform(labelSelectionModel->selectedItems(), std::back_inserter(viewItems), [=, this](dspx::Label *item) {
            auto viewItem = q->getLabelViewItemFromDocumentItem(item);
            Q_ASSERT(viewItem);
            return viewItem;
        });
        return viewItems;
    }

    QObjectList LabelSelectionController::getItemsBetween(QObject *startItem, QObject *endItem) const {
        auto startDocumentItem = startItem ? q->getLabelDocumentItemFromViewItem(qobject_cast<sflow::LabelViewModel *>(startItem)) : labelSequence->firstItem();
        auto endDocumentItem = endItem ? q->getLabelDocumentItemFromViewItem(qobject_cast<sflow::LabelViewModel *>(endItem)) : labelSequence->lastItem();
        Q_ASSERT(startDocumentItem && endDocumentItem);
        QObjectList viewItems;
        if (startDocumentItem->pos() > endDocumentItem->pos()) {
            std::swap(startDocumentItem, endDocumentItem);
        }
        for (auto item = startDocumentItem; item; item = labelSequence->nextItem(item)) {
            auto viewItem = q->getLabelViewItemFromDocumentItem(item);
            Q_ASSERT(viewItem);
            viewItems.append(viewItem);
            if (item == endDocumentItem) {
                break;
            }
        }
        return viewItems;
    }

    void LabelSelectionController::select(QObject *item, SelectionCommand command) {
        qCDebug(lcLabelSelectionController) << "Label view item selected" << item << command;
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
        auto documentItem = q->getLabelDocumentItemFromViewItem(qobject_cast<sflow::LabelViewModel *>(item));
        Q_ASSERT(!item || documentItem);
        selectionModel->select(documentItem, documentSelectionCommand, dspx::SelectionModel::ST_Label);
    }

    QObject *LabelSelectionController::currentItem() const {
        return q->getLabelViewItemFromDocumentItem(labelSelectionModel->currentItem());
    }

    bool LabelSelectionController::editScopeFocused() const {
        return selectionModel->selectionType() == dspx::SelectionModel::ST_Label;
    }

}
