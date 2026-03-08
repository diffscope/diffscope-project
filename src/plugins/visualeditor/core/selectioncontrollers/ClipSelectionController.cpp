#include "ClipSelectionController_p.h"

#include <algorithm>
#include <iterator>

#include <QLoggingCategory>

#include <ScopicFlowCore/ClipViewModel.h>

#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipSelectionModel.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/Timeline.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcClipSelectionController, "diffscope.visualeditor.clipselectioncontroller")

    ClipSelectionController::ClipSelectionController(ProjectViewModelContext *parent)
        : SelectionController(parent), q(parent) {
        const auto documentContext = q->windowHandle()->projectDocumentContext();
        auto document = documentContext->document();
        selectionModel = document->selectionModel();
        clipSelectionModel = selectionModel->clipSelectionModel();
        trackList = document->model()->tracks();
        connect(clipSelectionModel, &dspx::ClipSelectionModel::currentItemChanged, this, &SelectionController::currentItemChanged);
        connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, this, &SelectionController::editScopeFocusedChanged);
    }

    ClipSelectionController::~ClipSelectionController() = default;

    QObjectList ClipSelectionController::getSelectedItems() const {
        QObjectList viewItems;
        std::ranges::transform(clipSelectionModel->selectedItems(), std::back_inserter(viewItems), [=, this](dspx::Clip *item) {
            auto viewItem = q->getClipViewItemFromDocumentItem(item);
            Q_ASSERT(viewItem);
            return viewItem;
        });
        return viewItems;
    }

    QObjectList ClipSelectionController::getItemsBetween(QObject *startItem, QObject *endItem) const {
        auto startDocumentItem = startItem ? q->getClipDocumentItemFromViewItem(qobject_cast<sflow::ClipViewModel *>(startItem)) : nullptr;
        auto endDocumentItem = endItem ? q->getClipDocumentItemFromViewItem(qobject_cast<sflow::ClipViewModel *>(endItem)) : nullptr;

        if (!startDocumentItem || !endDocumentItem) {
            return {};
        }

        const auto tracks = trackList->items();
        const auto startTrack = startDocumentItem->clipSequence()->track();
        const auto endTrack = endDocumentItem->clipSequence()->track();
        const int startIndex = tracks.indexOf(startTrack);
        const int endIndex = tracks.indexOf(endTrack);
        if (startIndex < 0 || endIndex < 0) {
            return {};
        }

        const int fromTrack = std::min(startIndex, endIndex);
        const int toTrack = std::max(startIndex, endIndex);
        const int fromPosition = std::min(startDocumentItem->position(), endDocumentItem->position());
        const int toPosition = std::max(startDocumentItem->position(), endDocumentItem->position());

        QObjectList viewItems;
        for (int trackIndex = fromTrack; trackIndex <= toTrack; ++trackIndex) {
            auto sequence = tracks.at(trackIndex)->clips();
            for (auto clip : sequence->asRange()) {
                const bool sameTrack = (trackIndex == startIndex) && (trackIndex == endIndex);
                if (sameTrack) {
                    if (clip->position() < fromPosition || clip->position() > toPosition) {
                        continue;
                    }
                } else if (trackIndex == fromTrack && startIndex < endIndex) {
                    if (clip->position() < fromPosition) {
                        continue;
                    }
                } else if (trackIndex == fromTrack && startIndex > endIndex) {
                    if (clip->position() > toPosition) {
                        continue;
                    }
                } else if (trackIndex == toTrack && startIndex < endIndex) {
                    if (clip->position() > toPosition) {
                        continue;
                    }
                } else if (trackIndex == toTrack && startIndex > endIndex) {
                    if (clip->position() < fromPosition) {
                        continue;
                    }
                }

                auto viewItem = q->getClipViewItemFromDocumentItem(clip);
                if (!viewItem) {
                    continue;
                }
                viewItems.append(viewItem);
            }
        }
        return viewItems;
    }

    void ClipSelectionController::select(QObject *item, SelectionCommand command) {
        qCDebug(lcClipSelectionController) << "Clip view item selected" << item << command;
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
        auto documentItem = q->getClipDocumentItemFromViewItem(qobject_cast<sflow::ClipViewModel *>(item));
        Q_ASSERT(!item || documentItem);
        selectionModel->select(documentItem, documentSelectionCommand, dspx::SelectionModel::ST_Clip);
    }

    QObject *ClipSelectionController::currentItem() const {
        return q->getClipViewItemFromDocumentItem(clipSelectionModel->currentItem());
    }

    bool ClipSelectionController::editScopeFocused() const {
        return selectionModel->selectionType() == dspx::SelectionModel::ST_Clip;
    }

}
