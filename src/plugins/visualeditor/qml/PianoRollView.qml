import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

import QActionKit

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views

Item {
    id: view

    required property QtObject addOn
    required property PianoRollPanelInterface pianoRollPanelInterface

    readonly property ProjectViewModelContext projectViewModelContext: addOn?.windowHandle.ProjectViewModelContext.context ?? null

    anchors.fill: parent

    readonly property Timeline timeline: timeline

    TimelineContextMenuHelper {
        timeline: view.timeline
        window: view.Window.window
        projectTimeline: view.pianoRollPanelInterface?.windowHandle.projectTimeline ?? null
        document: view.pianoRollPanelInterface?.windowHandle.projectDocumentContext.document ?? null
    }

    SplitView {
        id: splitView
        anchors.fill: parent
        orientation: Qt.Horizontal

        Item {
            id: pianoArea
            SplitView.preferredWidth: 96
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: timeline.height
                }
                Clavier {
                    id: clavier
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clavierViewModel: view.pianoRollPanelInterface?.clavierViewModel ?? null
                    clavierInteractionController: view.pianoRollPanelInterface?.clavierInteractionController ?? null
                    scrollBehaviorViewModel: view.pianoRollPanelInterface?.scrollBehaviorViewModel ?? null
                }
            }
        }

        Item {
            id: noteArea
            SplitView.fillWidth: true
            clip: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Timeline {
                    id: timeline
                    Layout.fillWidth: true
                    timeViewModel: view.pianoRollPanelInterface?.timeViewModel ?? null
                    timeLayoutViewModel: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
                    playbackViewModel: view.projectViewModelContext?.playbackViewModel ?? null
                    scrollBehaviorViewModel: view.pianoRollPanelInterface?.scrollBehaviorViewModel ?? null
                    timelineInteractionController: view.pianoRollPanelInterface?.timelineInteractionController ?? null

                    property bool dragging: false
                    property bool adjustingLoop: false
                    property int loopAdjustmentOperation: 0

                    // TODO move this to C++ code
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

                        function onRubberBandDraggingStarted() {
                            timeline.timeLayoutViewModel.cursorPosition = -1
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
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: Theme.paneSeparatorColor
                }
                Item {
                    id: pianoRollViewContainer
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    PianoRollBackground {
                        anchors.fill: parent
                        timeViewModel: view.pianoRollPanelInterface?.timeViewModel ?? null
                        timeLayoutViewModel: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
                        clavierViewModel: view.pianoRollPanelInterface?.clavierViewModel ?? null
                    }
                    PianoRollScrollLayer {
                        anchors.fill: parent
                        timeViewModel: view.pianoRollPanelInterface?.timeViewModel ?? null
                        timeLayoutViewModel: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
                        clavierViewModel: view.pianoRollPanelInterface?.clavierViewModel ?? null
                        scrollBehaviorViewModel: view.pianoRollPanelInterface?.scrollBehaviorViewModel ?? null
                    }
                }
            }
            PositionIndicators {
                id: positionIndicators
                anchors.fill: parent
                anchors.topMargin: timeline.height
                timeViewModel: view.pianoRollPanelInterface?.timeViewModel ?? null
                playbackViewModel: view.projectViewModelContext?.playbackViewModel ?? null
                timeLayoutViewModel: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
            }
            HoverHandler {
                enabled: !(view.pianoRollPanelInterface?.mouseTrackingDisabled)
                cursorShape: undefined
                readonly property TimeManipulator timeManipulator: TimeManipulator {
                    id: timeManipulator
                    target: parent
                    timeViewModel: view.pianoRollPanelInterface?.timeViewModel ?? null
                    timeLayoutViewModel: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
                }
                onHoveredChanged: () => {
                    if (!hovered) {
                        view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = -1
                    }
                }
                onPointChanged: () => {
                    view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = timeManipulator.alignPosition(timeManipulator.mapToPosition(point.position.x), ScopicFlow.AO_Visible)
                }
                onEnabledChanged: {
                    if (!enabled) {
                        view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = -1
                    }
                }
            }
        }
    }
}
