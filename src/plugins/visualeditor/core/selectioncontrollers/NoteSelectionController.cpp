#include "NoteSelectionController_p.h"

#include <algorithm>
#include <iterator>

#include <QLoggingCategory>

#include <ScopicFlowCore/NoteViewModel.h>

#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcNoteSelectionController, "diffscope.visualeditor.noteselectioncontroller")

    NoteSelectionController::NoteSelectionController(ProjectViewModelContext *parent)
        : SelectionController(parent), q(parent) {
        const auto documentContext = q->windowHandle()->projectDocumentContext();
        auto document = documentContext->document();
        selectionModel = document->selectionModel();
        noteSelectionModel = selectionModel->noteSelectionModel();
        connect(noteSelectionModel, &dspx::NoteSelectionModel::currentItemChanged, this, &SelectionController::currentItemChanged);
        connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, this, &SelectionController::editScopeFocusedChanged);
    }

    NoteSelectionController::~NoteSelectionController() = default;

    QObjectList NoteSelectionController::getSelectedItems() const {
        QObjectList viewItems;
        std::ranges::transform(noteSelectionModel->selectedItems(), std::back_inserter(viewItems), [=, this](dspx::Note *item) {
            auto viewItem = q->getNoteViewItemFromDocumentItem(item);
            Q_ASSERT(viewItem);
            return viewItem;
        });
        return viewItems;
    }

    QObjectList NoteSelectionController::getItemsBetween(QObject *startItem, QObject *endItem) const {
        auto startDocumentItem = startItem ? q->getNoteDocumentItemFromViewItem(qobject_cast<sflow::NoteViewModel *>(startItem)) : nullptr;
        auto endDocumentItem = endItem ? q->getNoteDocumentItemFromViewItem(qobject_cast<sflow::NoteViewModel *>(endItem)) : nullptr;

        if (!startDocumentItem || !endDocumentItem) {
            return {};
        }

        auto sequence = startDocumentItem->noteSequence();
        if (sequence != endDocumentItem->noteSequence()) {
            return {};
        }

        if (startDocumentItem->pos() > endDocumentItem->pos()) {
            std::swap(startDocumentItem, endDocumentItem);
        }

        QObjectList viewItems;
        for (auto item = startDocumentItem; item; item = sequence->nextItem(item)) {
            auto viewItem = q->getNoteViewItemFromDocumentItem(item);
            Q_ASSERT(viewItem);
            viewItems.append(viewItem);
            if (item == endDocumentItem) {
                break;
            }
        }
        return viewItems;
    }

    void NoteSelectionController::select(QObject *item, SelectionCommand command) {
        qCDebug(lcNoteSelectionController) << "Note view item selected" << item << command;
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
        auto documentItem = q->getNoteDocumentItemFromViewItem(qobject_cast<sflow::NoteViewModel *>(item));
        Q_ASSERT(!item || documentItem);
        selectionModel->select(documentItem, documentSelectionCommand, dspx::SelectionModel::ST_Note);
    }

    QObject *NoteSelectionController::currentItem() const {
        return q->getNoteViewItemFromDocumentItem(noteSelectionModel->currentItem());
    }

    bool NoteSelectionController::editScopeFocused() const {
        return selectionModel->selectionType() == dspx::SelectionModel::ST_Note;
    }

}
