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

import DiffScope.Core
import DiffScope.DspxModel as DspxModel

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

        Pane {
            id: trackOverlaySelector
            SplitView.minimumWidth: 80
            SplitView.preferredWidth: 200
            visible: view.addOn?.trackSelectorVisible ?? false
            ScrollView {
                anchors.fill: parent
                ColumnLayout {
                    width: parent.width
                    spacing: 1
                    Repeater {
                        model: DelegateModel {
                            model: view.pianoRollPanelInterface?.trackOverlaySelectorModel ?? null
                            delegate: RowLayout {
                                id: trackRow
                                required property int index
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28
                                Rectangle {
                                    implicitWidth: 8
                                    Layout.fillHeight: true
                                    color: CoreInterface.trackColorSchema.colors[trackRow.modelData.display.track.colorId]
                                }
                                Label {
                                    Layout.alignment: Qt.AlignVCenter
                                    text: (trackRow.index + 1).toLocaleString()
                                }
                                ToolButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    implicitWidth: 20
                                    implicitHeight: 20
                                    padding: 2
                                    checkable: true
                                    icon.source: checked ? "image://fluent-system-icons/eye" : "image://fluent-system-icons/eye_off"
                                    text: qsTr("Show")
                                    display: AbstractButton.IconOnly
                                    checked: trackRow.modelData.display.overlayVisible
                                    onClicked: trackRow.modelData.display.overlayVisible = checked
                                }
                                ToolButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    implicitWidth: 20
                                    implicitHeight: 20
                                    padding: 2
                                    icon.source: "image://fluent-system-icons/edit"
                                    highlighted: view.pianoRollPanelInterface?.editingClip?.clipSequence?.track === trackRow.modelData.display.track
                                    enabled: (trackRow.modelData.display.track?.clips.size ?? 0) > 0
                                    onClicked: () => {
                                        // TODO consider playback position
                                        let clip = trackRow.modelData.display.track.clips.firstItem
                                        for (; clip && clip.type !== DspxModel.Clip.Singing; clip = trackRow.modelData.display.track.clips.nextItem(clip));
                                        if (!clip)
                                            return
                                        view.pianoRollPanelInterface.editingClip = clip
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignVCenter
                                    text: trackRow.modelData.display.track.name
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {
            id: pianoArea
            SplitView.preferredWidth: 96
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
                            actionId: "org.diffscope.visualeditor.pianoRollPanelTimelineToolBar"
                            context: view.pianoRollPanelInterface?.windowHandle.actionContext ?? null
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
                            readonly property double itemSize: 14
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
                                        spacing: 2
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
                                        padding: 1
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
                Clavier {
                    id: clavier
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clavierViewModel: view.pianoRollPanelInterface?.clavierViewModel ?? null
                    clavierInteractionController: view.pianoRollPanelInterface?.clavierInteractionController ?? null
                    scrollBehaviorViewModel: view.pianoRollPanelInterface?.scrollBehaviorViewModel ?? null

                    Connections {
                        target: clavier.clavierInteractionController
                        function onHoverEntered(clavier_, key) {
                            if (clavier_ === clavier) {
                                clavier.clavierViewModel.cursorPosition = key
                            }
                        }

                        function onHoverExited(clavier_, key) {
                            if (clavier_ === clavier) {
                                clavier.clavierViewModel.cursorPosition = -1
                            }
                        }
                    }
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
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: false
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
                readonly property ClavierManipulator clavierManipulator: ClavierManipulator {
                    id: clavierManipulator
                    target: parent
                    clavierViewModel: view.pianoRollPanelInterface?.clavierViewModel ?? null
                }
                onHoveredChanged: () => {
                    if (!hovered) {
                        view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = -1
                        view.pianoRollPanelInterface.clavierViewModel.cursorPosition = -1
                    }
                }
                onPointChanged: () => {
                    let p = noteArea.mapToItem(pianoRollViewContainer, point.position)
                    if (p.x < 0) {
                        view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = -1
                    } else {
                        view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = timeManipulator.alignPosition(timeManipulator.mapToPosition(p.x), ScopicFlow.AO_Visible)
                    }
                    if (p.y < 0) {
                        view.pianoRollPanelInterface.clavierViewModel.cursorPosition = -1
                    } else {
                        let i = Math.floor(clavierManipulator.mapToPosition(p.y))
                        if (i >= 128) i = -1
                        view.pianoRollPanelInterface.clavierViewModel.cursorPosition = i
                    }

                }
                onEnabledChanged: {
                    if (!enabled) {
                        view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = -1
                        view.pianoRollPanelInterface.clavierViewModel.cursorPosition = -1
                    }
                }
            }
        }
    }
}
