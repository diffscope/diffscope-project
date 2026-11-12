// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views

Timeline {
    id: timeline

    required property int layerPoint
    property ArrangementPanelInterface arrangementPanelInterface: null
    property PianoRollPanelInterface pianoRollPanelInterface: null

    signal selectionStarted()
    signal selectionFinished()

    ViewLayerHost {
        anchors.fill: parent
        point: timeline.layerPoint
        arrangementPanelInterface: timeline.arrangementPanelInterface
        pianoRollPanelInterface: timeline.pianoRollPanelInterface
    }

    property bool dragging: false
    property bool adjustingLoop: false
    property int loopAdjustmentOperation: 0

    Connections {
        target: timeline.playbackViewModel
        enabled: timeline.dragging
        function onPrimaryPositionChanged() {
            timeline.timeLayoutViewModel.cursorPosition = timeline.playbackViewModel.primaryPosition
        }
    }

    Connections {
        target: timeline.playbackViewModel
        enabled: timeline.adjustingLoop
        function onLoopStartChanged() {
            if (timeline.loopAdjustmentOperation === TimelineInteractionController.AdjustRange || timeline.loopAdjustmentOperation === TimelineInteractionController.AdjustStart) {
                timeline.timeLayoutViewModel.cursorPosition = timeline.playbackViewModel.loopStart
            } else if (timeline.loopAdjustmentOperation === TimelineInteractionController.AdjustEnd) {
                timeline.timeLayoutViewModel.cursorPosition = timeline.playbackViewModel.loopStart + timeline.playbackViewModel.loopLength
            }
        }

        function onLoopLengthChanged() {
            if (timeline.loopAdjustmentOperation === TimelineInteractionController.AdjustEnd) {
                timeline.timeLayoutViewModel.cursorPosition = timeline.playbackViewModel.loopStart + timeline.playbackViewModel.loopLength
            }
        }
    }

    Connections {
        target: timeline.timelineInteractionController
        function onPositionIndicatorMovingStarted(timeline_) {
            if (timeline_ === timeline) {
                timeline.dragging = true
            }
        }
        function onPositionIndicatorMovingFinished(timeline_) {
            if (timeline_ === timeline) {
                timeline.dragging = false
            }
        }

        function onRubberBandDraggingStarted(timeline_) {
            if (timeline_ === timeline)
                timeline.selectionStarted()
        }

        function onRubberBandDraggingCommitted(timeline_) {
            if (timeline_ === timeline)
                timeline.selectionFinished()
        }

        function onRubberBandDraggingAborted(timeline_) {
            if (timeline_ === timeline)
                timeline.selectionFinished()
        }

        function onLoopRangeAdjustingStarted(_, adjustmentOperation) {
            timeline.adjustingLoop = true
            timeline.loopAdjustmentOperation = adjustmentOperation
        }

        function onLoopRangeAdjustingFinished() {
            timeline.adjustingLoop = false
        }
    }
}
