// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views

ClipEditLayer {
    id: clipPane

    readonly property ArrangementPanelInterface arrangementPanelInterface: ViewLayer.arrangementPanelInterface
    readonly property PianoRollPanelInterface pianoRollPanelInterface: ViewLayer.pianoRollPanelInterface
    readonly property ProjectViewModelContext projectViewModelContext: ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
    readonly property bool clipCursorActive: clipStartBinding.when || clipEndBinding.when

    signal selectionStarted()
    signal selectionFinished()

    TrackListManipulator {
        id: trackListManipulator
        target: clipPane
        trackListViewModel: clipPane.trackListViewModel
        trackListLayoutViewModel: clipPane.trackListLayoutViewModel
    }

    trackListViewModel: clipPane.projectViewModelContext?.trackListViewModel ?? null
    trackListLayoutViewModel: clipPane.arrangementPanelInterface?.trackListLayoutViewModel ?? null
    scrollBehaviorViewModel: clipPane.arrangementPanelInterface?.scrollBehaviorViewModel ?? null
    selectionController: clipPane.projectViewModelContext?.clipSelectionController ?? null
    timeViewModel: clipPane.arrangementPanelInterface?.timeViewModel ?? null
    timeLayoutViewModel: clipPane.arrangementPanelInterface?.timeLayoutViewModel ?? null
    clipSequenceViewModel: clipPane.projectViewModelContext?.clipSequenceViewModel ?? null
    clipPaneInteractionController: clipPane.arrangementPanelInterface?.clipPaneInteractionController ?? null
    thumbnailComponent: ArrangementViewThumbnail {
        arrangementPanelInterface: clipPane.arrangementPanelInterface
        pianoRollPanelInterface: clipPane.pianoRollPanelInterface
        viewport: clipPane
        trackTop: trackListManipulator.map[clipViewModel?.trackIndex ?? -1] ?? 0
    }

    property ClipViewModel activeClip: null
    property bool drawingClip: false

    function finishClipEdit(layer) {
        if (layer !== clipPane)
            return
        clipStartBinding.when = false
        clipEndBinding.when = false
        clipPane.drawingClip = false
        clipPane.activeClip = null
    }

    Binding {
        id: clipStartBinding
        when: false
        restoreMode: Binding.RestoreNone
        clipPane.timeLayoutViewModel.cursorPosition: clipPane.activeClip.position
    }

    Binding {
        id: clipEndBinding
        when: false
        restoreMode: Binding.RestoreNone
        clipPane.timeLayoutViewModel.cursorPosition: clipPane.activeClip.position + clipPane.activeClip.length
    }

    Connections {
        target: clipPane.clipSequenceViewModel
        enabled: clipPane.drawingClip

        function onItemInserted(item) {
            if (!clipPane.drawingClip || clipPane.activeClip)
                return
            clipPane.activeClip = item
            clipEndBinding.when = true
        }
    }

    Connections {
        target: clipPane.clipPaneInteractionController
        function onMovingStarted(clipPane_, clipViewModel) {
            if (clipPane_ === clipPane) {
                clipPane.activeClip = clipViewModel
                clipStartBinding.when = true
            }
        }
        function onMovingCommitted(clipPane_) {
            clipPane.finishClipEdit(clipPane_)
        }
        function onMovingAborted(clipPane_) {
            clipPane.finishClipEdit(clipPane_)
        }
        function onAdjustLengthStarted(clipPane_, clipViewModel, edge) {
            if (clipPane_ === clipPane) {
                clipPane.activeClip = clipViewModel
                if (edge === ClipPaneInteractionController.LeftEdge) {
                    clipStartBinding.when = true
                } else {
                    clipEndBinding.when = true
                }
            }
        }
        function onAdjustLengthCommitted(clipPane_) {
            clipPane.finishClipEdit(clipPane_)
        }
        function onAdjustLengthAborted(clipPane_) {
            clipPane.finishClipEdit(clipPane_)
        }
        function onSplitStarted(clipPane_, position) {
            if (clipPane_ === clipPane) {
                clipPane.timeLayoutViewModel.cursorPosition = position
            }
        }
        function onDrawingStarted(clipPane_) {
            if (clipPane_ === clipPane) {
                clipPane.drawingClip = true
                clipPane.activeClip = null
            }
        }
        function onDrawingCommitted(clipPane_) {
            clipPane.finishClipEdit(clipPane_)
        }
        function onDrawingAborted(clipPane_) {
            clipPane.finishClipEdit(clipPane_)
        }
        function onRubberBandDraggingStarted(clipPane_) {
            if (clipPane_ === clipPane)
                clipPane.selectionStarted()
        }
        function onRubberBandDraggingCommitted(clipPane_) {
            if (clipPane_ === clipPane)
                clipPane.selectionFinished()
        }
        function onRubberBandDraggingAborted(clipPane_) {
            if (clipPane_ === clipPane)
                clipPane.selectionFinished()
        }
    }
}
