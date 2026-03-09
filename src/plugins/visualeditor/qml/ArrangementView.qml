import QtQml
import QtQml.Models
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
    required property ArrangementPanelInterface arrangementPanelInterface

    readonly property ProjectViewModelContext projectViewModelContext: addOn?.windowHandle.ProjectViewModelContext.context ?? null

    anchors.fill: parent

    readonly property Timeline timeline: timeline
    readonly property Item centerEditArea: clipViewContainer

    TimelineContextMenuHelper {
        timeline: view.timeline
        window: view.Window.window
        projectTimeline: view.arrangementPanelInterface?.windowHandle.projectTimeline ?? null
        document: view.arrangementPanelInterface?.windowHandle.projectDocumentContext.document ?? null
    }

    Menu {
        id: trackItemContextMenu
        MenuActionInstantiator {
            actionId: "org.diffscope.core.trackItemContextMenu"
            context: view.addOn?.windowHandle.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: trackSceneContextMenu
        MenuActionInstantiator {
            actionId: "org.diffscope.core.trackSceneContextMenu"
            context: view.addOn?.windowHandle.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Connections {
        target: view.arrangementPanelInterface?.trackListInteractionController ?? null
        function onContextMenuRequested(trackList_) {
            if (trackList_ !== trackList)
                return
            trackSceneContextMenu.popup()

        }

        function onItemContextMenuRequested(trackList_) {
            if (trackList_ !== trackList)
                return
            trackItemContextMenu.popup()
        }
    }

    Menu {
        id: clipItemContextMenu
        MenuActionInstantiator {
            actionId: "org.diffscope.core.clipItemContextMenu"
            context: view.addOn?.windowHandle.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: clipSceneContextMenu
        MenuActionInstantiator {
            actionId: "org.diffscope.core.clipSceneContextMenu"
            context: view.addOn?.windowHandle.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Connections {
        target: view.arrangementPanelInterface?.clipPaneInteractionController ?? null
        function onContextMenuRequested(clipPane_) {
            if (clipPane_ !== clipPane)
                return
            clipSceneContextMenu.popup()
        }

        function onItemContextMenuRequested(clipPane_) {
            if (clipPane_ !== clipPane)
                return
            clipItemContextMenu.popup()
        }
    }

    SplitView {
        id: splitView
        anchors.fill: parent
        orientation: Qt.Horizontal
        readonly property bool rightAligned: EditorPreference.trackListOnRight
        readonly property Item trackArea: Item {
            id: trackArea
            SplitView.preferredWidth: 360
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                ToolBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: timeline.height
                    padding: 4
                    ToolBarContainer {
                        id: toolBar
                        anchors.fill: parent
                        property MenuActionInstantiator instantiator: MenuActionInstantiator {
                            actionId: "org.diffscope.visualeditor.arrangementPanelTimelineToolBar"
                            context: view.arrangementPanelInterface?.windowHandle.actionContext ?? null
                            separatorComponent: ToolBarContainerSeparator {
                            }
                            stretchComponent: ToolBarContainerStretch {
                            }
                            Component.onCompleted: forceUpdateLayouts()
                        }
                        toolButtonComponent: ToolButton {
                            display: icon.source.toString().length !== 0 ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
                            DescriptiveText.bindAccessibleDescription: true
                        }
                    }
                }
                AdditionalTrackNavigator {
                    id: additionalTrackNavigator
                    additionalTrackLoader: view.addOn?.additionalTrackLoader ?? null
                    associatedPane: additionalTrackPane
                    Layout.fillWidth: true
                }
                Item {
                    id: trackListContainer
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    TrackList {
                        id: trackList
                        anchors.fill: parent
                        trackListViewModel: view.projectViewModelContext?.trackListViewModel ?? null
                        trackListLayoutViewModel: view.arrangementPanelInterface?.trackListLayoutViewModel ?? null
                        scrollBehaviorViewModel: view.arrangementPanelInterface?.scrollBehaviorViewModel ?? null
                        trackListInteractionController: view.arrangementPanelInterface?.trackListInteractionController ?? null
                        selectionController: view.projectViewModelContext?.trackSelectionController ?? null
                        rightAligned: splitView.rightAligned
                    }
                }
            }
        }
        readonly property Item clipArea: Item {
            id: clipArea
            SplitView.fillWidth: true
            clip: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Timeline {
                    id: timeline
                    Layout.fillWidth: true
                    timeViewModel: view.arrangementPanelInterface?.timeViewModel ?? null
                    timeLayoutViewModel: view.arrangementPanelInterface?.timeLayoutViewModel ?? null
                    playbackViewModel: view.projectViewModelContext?.playbackViewModel ?? null
                    scrollBehaviorViewModel: view.arrangementPanelInterface?.scrollBehaviorViewModel ?? null
                    timelineInteractionController: view.arrangementPanelInterface?.timelineInteractionController ?? null

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
                AdditionalTrackPane {
                    id: additionalTrackPane
                    additionalTrackLoader: view.addOn?.additionalTrackLoader ?? null
                    Layout.fillWidth: true
                }
                Item {
                    id: clipViewContainer
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ClipPane {
                        id: clipPane
                        anchors.fill: parent
                        trackListViewModel: view.projectViewModelContext?.trackListViewModel ?? null
                        trackListLayoutViewModel: view.arrangementPanelInterface?.trackListLayoutViewModel ?? null
                        scrollBehaviorViewModel: view.arrangementPanelInterface?.scrollBehaviorViewModel ?? null
                        selectionController: view.projectViewModelContext?.clipSelectionController ?? null
                        timeViewModel: view.arrangementPanelInterface?.timeViewModel ?? null
                        timeLayoutViewModel: view.arrangementPanelInterface?.timeLayoutViewModel ?? null
                        clipSequenceViewModel: view.projectViewModelContext?.clipSequenceViewModel ?? null
                        clipPaneInteractionController: view.arrangementPanelInterface?.clipPaneInteractionController ?? null
                        thumbnailComponent: ArrangementViewThumbnail {
                            pianoRollPanelInterface: view.addOn?.pianoRollPanelInterface ?? null
                            projectViewModelContext: view.projectViewModelContext
                        }

                        property ClipViewModel activeClip: null

                        Binding {
                            id: clipStartBinding
                            when: false
                            clipPane.timeLayoutViewModel.cursorPosition: clipPane.activeClip.position
                        }

                        Binding {
                            id: clipEndBinding
                            when: false
                            clipPane.timeLayoutViewModel.cursorPosition: clipPane.activeClip.position + clipPane.activeClip.length
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
                                if (clipPane_ === clipPane) {
                                    clipStartBinding.when = false
                                    clipPane.activeClip = null
                                }
                            }
                            function onMovingAborted(clipPane_) {
                                if (clipPane_ === clipPane) {
                                    clipStartBinding.when = false
                                    clipPane.activeClip = null
                                }
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
                                if (clipPane_ === clipPane) {
                                    clipStartBinding.when = clipEndBinding.when = false
                                    clipPane.activeClip = null
                                }
                            }
                            function onAdjustLengthAborted(clipPane_) {
                                if (clipPane_ === clipPane) {
                                    clipStartBinding.when = clipEndBinding.when = false
                                    clipPane.activeClip = null
                                }
                            }
                            function onSplitStarted(clipPane_, position) {
                                if (clipPane_ === clipPane) {
                                    clipPane.timeLayoutViewModel.cursorPosition = position
                                }
                            }
                            function onRubberBandDraggingStarted(clipPane_) {
                                if (clipPane_ === clipPane) {
                                    clipPane.timeLayoutViewModel.cursorPosition = -1
                                }
                            }
                        }
                    }
                }
            }
            PositionIndicators {
                id: positionIndicators
                anchors.fill: parent
                anchors.topMargin: timeline.height
                timeViewModel: view.arrangementPanelInterface?.timeViewModel ?? null
                playbackViewModel: view.projectViewModelContext?.playbackViewModel ?? null
                timeLayoutViewModel: view.arrangementPanelInterface?.timeLayoutViewModel ?? null
            }
            HoverHandler {
                enabled: !(view.arrangementPanelInterface?.mouseTrackingDisabled)
                cursorShape: undefined
                readonly property TimeManipulator timeManipulator: TimeManipulator {
                    id: timeManipulator
                    target: parent
                    timeViewModel: view.arrangementPanelInterface?.timeViewModel ?? null
                    timeLayoutViewModel: view.arrangementPanelInterface?.timeLayoutViewModel ?? null
                }
                onHoveredChanged: () => {
                    if (!hovered) {
                        view.arrangementPanelInterface.timeLayoutViewModel.cursorPosition = -1
                    }
                }
                onPointChanged: () => {
                    view.arrangementPanelInterface.timeLayoutViewModel.cursorPosition = timeManipulator.alignPosition(timeManipulator.mapToPosition(point.position.x), ScopicFlow.AO_Visible)
                }
                onEnabledChanged: {
                    if (!enabled) {
                        view.arrangementPanelInterface.timeLayoutViewModel.cursorPosition = -1
                    }
                }
            }
        }

        function updateContainer() {
            while (count) {
                takeItem(0)
            }
            if (rightAligned) {
                addItem(clipArea)
                addItem(trackArea)
            } else {
                addItem(trackArea)
                addItem(clipArea)
            }
        }

        Component.onCompleted: updateContainer()
        onRightAlignedChanged: updateContainer()
    }

}