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

    TimelineContextMenuHelper {
        timeline: view.timeline
        window: view.Window.window
        projectTimeline: view.arrangementPanelInterface?.windowHandle.projectTimeline ?? null
        document: view.arrangementPanelInterface?.windowHandle.projectDocumentContext.document ?? null
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
                    padding: 2
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
                            implicitWidth: 20
                            implicitHeight: 20
                            display: icon.source.toString().length !== 0 ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
                            DescriptiveText.bindAccessibleDescription: true
                        }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: Theme.paneSeparatorColor
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    spacing: 0
                    Repeater {
                        model: view.addOn?.additionalTrackLoader.loadedComponents ?? []
                        ColumnLayout {
                            id: layout
                            required property string modelData
                            required property int index
                            readonly property Item item: {
                                let a = additionalTrackRepeater.count - additionalTrackRepeater.count
                                return additionalTrackRepeater.itemAt(a + index)?.item ?? null
                            }
                            readonly property double itemSize: 12
                            Layout.fillWidth: true
                            spacing: 0
                            Item {
                                id: container
                                Layout.fillWidth: true
                                Layout.preferredHeight: layout.item?.height ?? 0
                                readonly property Action moveUpAction: Action {
                                    enabled: layout.index !== 0
                                    text: qsTr("Move Up")
                                    icon.source: "image://fluent-system-icons/arrow_up"
                                    onTriggered: view.addOn.additionalTrackLoader.moveUp(layout.modelData)
                                }
                                readonly property Action moveDownAction: Action {
                                    enabled: layout.index !== (view.addOn?.additionalTrackLoader.loadedComponents.length ?? 0) - 1
                                    text: qsTr("Move Down")
                                    icon.source: "image://fluent-system-icons/arrow_down"
                                    onTriggered: view.addOn.additionalTrackLoader.moveDown(layout.modelData)
                                }
                                readonly property Action removeAction: Action {
                                    text: qsTr("Remove")
                                    icon.source: "image://fluent-system-icons/dismiss"
                                    onTriggered: view.addOn.additionalTrackLoader.removeItem(layout.modelData)
                                }
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    anchors.leftMargin: 4
                                    anchors.rightMargin: 4
                                    visible: (layout.item?.height ?? 0) >= 12
                                    IconLabel {
                                        Layout.fillHeight: true
                                        icon.height: layout.itemSize
                                        icon.width: layout.itemSize
                                        icon.source: layout.item?.ActionInstantiator.icon.source ?? ""
                                        icon.color: layout.item?.ActionInstantiator.icon.color.valid ? layout.item.ActionInstantiator.icon.color : Theme.foregroundPrimaryColor
                                        text: view.addOn.additionalTrackLoader.componentName(layout.modelData)
                                        color: Theme.foregroundPrimaryColor
                                        font.pixelSize: layout.itemSize * 0.75
                                    }
                                    Item {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                    }
                                    ToolButton {
                                        implicitWidth: layout.itemSize
                                        implicitHeight: layout.itemSize
                                        padding: 0
                                        visible: hoverHandler.hovered
                                        display: AbstractButton.IconOnly
                                        action: container.moveUpAction
                                    }
                                    ToolButton {
                                        implicitWidth: layout.itemSize
                                        implicitHeight: layout.itemSize
                                        padding: 0
                                        visible: hoverHandler.hovered
                                        display: AbstractButton.IconOnly
                                        action: container.moveDownAction
                                    }
                                    ToolButton {
                                        implicitWidth: layout.itemSize
                                        implicitHeight: layout.itemSize
                                        padding: 1 // its icon looks a bit larger, so we need to add some padding
                                        visible: hoverHandler.hovered
                                        display: AbstractButton.IconOnly
                                        action: container.removeAction
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.RightButton
                                    Menu {
                                        id: menu
                                        contentData: [container.moveUpAction, container.moveDownAction, container.removeAction]
                                    }
                                    onClicked: menu.popup()
                                }
                                HoverHandler {
                                    id: hoverHandler
                                }
                                DescriptiveText.activated: hoverHandler.hovered && (layout.item?.height ?? 0) < 12
                                DescriptiveText.toolTip: view.addOn.additionalTrackLoader.componentName(layout.modelData)
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 1
                                color: Theme.paneSeparatorColor
                            }
                        }
                    }
                }
                Item {
                    id: trackListContainer
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    TrackList {
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
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: Theme.paneSeparatorColor
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Repeater {
                        id: additionalTrackRepeater
                        model: view.addOn?.additionalTrackLoader.loadedComponents ?? []
                        ColumnLayout {
                            required property string modelData
                            required property int index
                            Layout.fillWidth: true
                            spacing: 0
                            readonly property Item separator: Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 1
                                color: Theme.paneSeparatorColor
                            }
                            readonly property Item item: view.addOn?.additionalTrackLoader.loadedItems[index] ?? null
                            data: [item, separator]
                        }
                    }
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