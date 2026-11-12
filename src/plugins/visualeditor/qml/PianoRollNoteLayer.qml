// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml.Models
import QtQuick

import dev.sjimo.ScopicFlow.Views

Item {
    id: noteLayer

    readonly property PianoRollPanelInterface pianoRollPanelInterface: ViewLayer.pianoRollPanelInterface
    readonly property ProjectViewModelContext projectViewModelContext: ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
    readonly property real bottomExpansion: Math.max(0, height - ViewLayer.visibleRect.height)

    enabled: !(noteLayer.pianoRollPanelInterface?.pitchToolActive ?? false)

    Repeater {
        model: DelegateModel {
            model: noteLayer.pianoRollPanelInterface?.trackOverlaySelectorModel ?? null
            delegate: NoteEditLayerSequence {
                required property int index
                required property var modelData
                readonly property bool isEditingTrack: noteLayer.pianoRollPanelInterface?.editingClip?.clipSequence?.track === modelData.display.track

                anchors.fill: parent
                selectionController: noteLayer.projectViewModelContext?.noteSelectionController ?? null
                timeViewModel: noteLayer.pianoRollPanelInterface?.timeViewModel ?? null
                timeLayoutViewModel: noteLayer.pianoRollPanelInterface?.timeLayoutViewModel ?? null
                clavierViewModel: noteLayer.pianoRollPanelInterface?.clavierViewModel ?? null
                noteEditLayerInteractionController: noteLayer.pianoRollPanelInterface?.noteEditLayerInteractionController ?? null
                // TODO this should not depend on signal connection order
                clipSequenceViewModel: noteLayer.projectViewModelContext?.getSingingClipPerTrackSequenceViewModel(modelData.display.track) ?? null
                trackListViewModel: noteLayer.projectViewModelContext?.trackListViewModel ?? null
                editingItem: noteLayer.projectViewModelContext?.getClipViewItemFromDocumentItem(noteLayer.pianoRollPanelInterface?.editingClip ?? null) ?? null
                z: isEditingTrack ? 1 : 0
                active: modelData.display.overlayVisible || isEditingTrack
                transparentDisplay: noteLayer.pianoRollPanelInterface?.pitchToolActive ?? false
                bottomExpansion: noteLayer.bottomExpansion
            }
        }
    }
}
