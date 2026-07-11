#include "KeySignatureSelectionController_p.h"

#include <algorithm>
#include <iterator>

#include <QLoggingCategory>
#include <QQuickWindow>

#include <ScopicFlowCore/LabelViewModel.h>

#include <dspxmodelORM/Model.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelSelectionModel/KeySignatureSelectionModel.h>
#include <dspxmodelORM/KeySignatureSequence.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcKeySignatureSelectionController, "diffscope.visualeditor.keysignatureselectionsontroller")

    KeySignatureSelectionController::KeySignatureSelectionController(ProjectViewModelContext *parent)
        : SelectionController(parent), q(parent) {
        keySignatureSequence = q->windowHandle()->projectDocumentContext()->document()->model()->keySignatures();
        selectionModel = q->windowHandle()->projectDocumentContext()->document()->selectionModel();
        keySignatureSelectionModel = q->windowHandle()->projectDocumentContext()->document()->selectionModel()->keySignatureSelectionModel();
        connect(keySignatureSelectionModel, &dspx::KeySignatureSelectionModel::currentItemChanged, this, &SelectionController::currentItemChanged);
        connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, this, &SelectionController::editScopeFocusedChanged);
    }

    KeySignatureSelectionController::~KeySignatureSelectionController() = default;

    QObjectList KeySignatureSelectionController::getSelectedItems() const {
        QObjectList viewItems;
        std::ranges::transform(keySignatureSelectionModel->selectedItems(), std::back_inserter(viewItems), [=, this](dspx::KeySignature *item) {
            auto viewItem = q->getKeySignatureViewItemFromDocumentItem(item);
            Q_ASSERT(viewItem);
            return viewItem;
        });
        return viewItems;
    }

    QObjectList KeySignatureSelectionController::getItemsBetween(QObject *startItem, QObject *endItem) const {
        auto startDocumentItem = startItem ? q->getKeySignatureDocumentItemFromViewItem(qobject_cast<sflow::LabelViewModel *>(startItem)) : keySignatureSequence->firstItem();
        auto endDocumentItem = endItem ? q->getKeySignatureDocumentItemFromViewItem(qobject_cast<sflow::LabelViewModel *>(endItem)) : keySignatureSequence->lastItem();
        Q_ASSERT(startDocumentItem && endDocumentItem);
        QObjectList viewItems;
        if (startDocumentItem->position() > endDocumentItem->position()) {
            std::swap(startDocumentItem, endDocumentItem);
        }
        for (auto item = startDocumentItem; item; item = item->nextItem()) {
            auto viewItem = q->getKeySignatureViewItemFromDocumentItem(item);
            Q_ASSERT(viewItem);
            viewItems.append(viewItem);
            if (item == endDocumentItem) {
                break;
            }
        }
        return viewItems;
    }

    void KeySignatureSelectionController::select(QObject *item, SelectionCommand command) {
        qCDebug(lcKeySignatureSelectionController) << "KeySignature view item selected" << item << command;
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
        auto documentItem = q->getKeySignatureDocumentItemFromViewItem(qobject_cast<sflow::LabelViewModel *>(item));
        Q_ASSERT(!item || documentItem);
        selectionModel->select(documentItem, documentSelectionCommand, dspx::SelectionModel::ST_KeySignature);
    }

    QObject *KeySignatureSelectionController::currentItem() const {
        return q->getKeySignatureViewItemFromDocumentItem(keySignatureSelectionModel->currentItem());
    }

    bool KeySignatureSelectionController::editScopeFocused() const {
        return selectionModel->selectionType() == dspx::SelectionModel::ST_KeySignature;
    }

}
