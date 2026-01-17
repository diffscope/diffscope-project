#include "TrackSelectionController_p.h"

#include <algorithm>
#include <iterator>

#include <QLoggingCategory>

#include <ScopicFlowCore/TrackViewModel.h>

#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/TrackSelectionModel.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Timeline.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/DspxDocument.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcTrackSelectionController, "diffscope.visualeditor.trackselectioncontroller")

    TrackSelectionController::TrackSelectionController(ProjectViewModelContext *parent)
        : SelectionController(parent), q(parent) {
        trackList = q->windowHandle()->projectDocumentContext()->document()->model()->tracks();
        selectionModel = q->windowHandle()->projectDocumentContext()->document()->selectionModel();
        trackSelectionModel = selectionModel->trackSelectionModel();
        connect(trackSelectionModel, &dspx::TrackSelectionModel::currentItemChanged, this, &SelectionController::currentItemChanged);
        connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, this, &SelectionController::editScopeFocusedChanged);
    }

    TrackSelectionController::~TrackSelectionController() = default;

    QObjectList TrackSelectionController::getSelectedItems() const {
        QObjectList viewItems;
        std::ranges::transform(trackSelectionModel->selectedItems(), std::back_inserter(viewItems), [=, this](dspx::Track *item) {
            auto viewItem = q->getTrackViewItemFromDocumentItem(item);
            Q_ASSERT(viewItem);
            return viewItem;
        });
        return viewItems;
    }

    QObjectList TrackSelectionController::getItemsBetween(QObject *startItem, QObject *endItem) const {
        auto startDocumentItem = startItem ? q->getTrackDocumentItemFromViewItem(qobject_cast<sflow::TrackViewModel *>(startItem)) : trackList->item(0);
        auto endDocumentItem = endItem ? q->getTrackDocumentItemFromViewItem(qobject_cast<sflow::TrackViewModel *>(endItem)) : trackList->item(trackList->size() - 1);
        Q_ASSERT(startDocumentItem && endDocumentItem);

        const auto &items = trackList->items();
        const int startIndex = items.indexOf(startDocumentItem);
        const int endIndex = items.indexOf(endDocumentItem);
        Q_ASSERT(startIndex >= 0 && endIndex >= 0);

        const int from = std::min(startIndex, endIndex);
        const int to = std::max(startIndex, endIndex);

        QObjectList viewItems;
        for (int i = from; i <= to; ++i) {
            auto viewItem = q->getTrackViewItemFromDocumentItem(items.at(i));
            Q_ASSERT(viewItem);
            viewItems.append(viewItem);
        }
        return viewItems;
    }

    void TrackSelectionController::select(QObject *item, SelectionCommand command) {
        qCDebug(lcTrackSelectionController) << "Track view item selected" << item << command;
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
        auto documentItem = q->getTrackDocumentItemFromViewItem(qobject_cast<sflow::TrackViewModel *>(item));
        Q_ASSERT(!item || documentItem);
        selectionModel->select(documentItem, documentSelectionCommand, dspx::SelectionModel::ST_Track);
    }

    QObject *TrackSelectionController::currentItem() const {
        return q->getTrackViewItemFromDocumentItem(trackSelectionModel->currentItem());
    }

    bool TrackSelectionController::editScopeFocused() const {
        return selectionModel->selectionType() == dspx::SelectionModel::ST_Track;
    }

}
