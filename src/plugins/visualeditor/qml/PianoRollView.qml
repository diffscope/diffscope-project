import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Templates as T

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
    readonly property Item centerEditArea: pianoRollViewContainerSplitViewport
    readonly property T.SplitView noteAreaSplitView: noteAreaSplitView

    readonly property double bottomExpansion: pianoRollViewContainer.height - pianoRollViewContainerSplitViewport.height

    TimelineContextMenuHelper {
        timeline: view.timeline
        window: view.Window.window
        projectTimeline: view.pianoRollPanelInterface?.windowHandle.projectTimeline ?? null
        document: view.pianoRollPanelInterface?.windowHandle.projectDocumentContext.document ?? null
    }

    Menu {
        id: noteItemContextMenu
        MenuActionInstantiator {
            actionId: "org.diffscope.core.noteItemContextMenu"
            context: view.addOn?.windowHandle.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: noteSceneContextMenu
        MenuActionInstantiator {
            actionId: "org.diffscope.core.noteSceneContextMenu"
            context: view.addOn?.windowHandle.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Connections {
        target: view.pianoRollPanelInterface?.noteEditLayerInteractionController ?? null
        function onContextMenuRequested() {
            noteSceneContextMenu.popup()
        }

        function onItemContextMenuRequested() {
            noteItemContextMenu.popup()
        }
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
                                        view.addOn.windowHandle.projectDocumentContext.document.selectionModel.select(null, DspxModel.SelectionModel.Select, DspxModel.SelectionModel.ST_Note, clip.notes);
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
                AdditionalTrackNavigator {
                    id: additionalTrackNavigator
                    additionalTrackLoader: view.addOn?.additionalTrackLoader ?? null
                    associatedPane: additionalTrackPane
                    Layout.fillWidth: true
                }
                Clavier {
                    id: clavier
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clavierViewModel: view.pianoRollPanelInterface?.clavierViewModel ?? null
                    clavierInteractionController: view.pianoRollPanelInterface?.clavierInteractionController ?? null
                    scrollBehaviorViewModel: view.pianoRollPanelInterface?.scrollBehaviorViewModel ?? null

                    bottomExpansion: view.bottomExpansion

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
                AdditionalTrackPane {
                    id: additionalTrackPane
                    additionalTrackLoader: view.addOn?.additionalTrackLoader ?? null
                    Layout.fillWidth: true
                }
                SplitView {
                    id: noteAreaSplitView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: Qt.Vertical
                    Item {
                        id: pianoRollViewContainerSplitViewport
                        SplitView.fillHeight: true
                        Item {
                            id: pianoRollViewContainer

                            width: parent.width
                            height: noteAreaSplitView.height

                            PianoRollBackground {
                                anchors.fill: parent
                                timeViewModel: view.pianoRollPanelInterface?.timeViewModel ?? null
                                timeLayoutViewModel: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
                                clavierViewModel: view.pianoRollPanelInterface?.clavierViewModel ?? null
                                scaleHighlightSequenceViewModel: view.pianoRollPanelInterface?.scaleHighlightEnabled ? (view.projectViewModelContext?.scaleHighlightSequenceViewModel ?? null) : null
                            }
                            Item {
                                id: noteEditLayerSequenceStack
                                anchors.fill: parent
                                Repeater {
                                    model: DelegateModel {
                                        model: view.pianoRollPanelInterface?.trackOverlaySelectorModel ?? null
                                        delegate: NoteEditLayerSequence {
                                            required property int index
                                            required property var modelData
                                            readonly property bool isEditingTrack:view.pianoRollPanelInterface?.editingClip?.clipSequence?.track === modelData.display.track
                                            anchors.fill: parent
                                            scrollBehaviorViewModel: view.pianoRollPanelInterface?.scrollBehaviorViewModel ?? null
                                            selectionController: view.projectViewModelContext?.noteSelectionController ?? null
                                            timeViewModel: view.pianoRollPanelInterface?.timeViewModel ?? null
                                            timeLayoutViewModel: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
                                            clavierViewModel: view.pianoRollPanelInterface?.clavierViewModel ?? null
                                            noteEditLayerInteractionController: view.pianoRollPanelInterface?.noteEditLayerInteractionController ?? null
                                            // TODO this should not depend on signal connection order
                                            clipSequenceViewModel: view.projectViewModelContext?.getSingingClipPerTrackSequenceViewModel(modelData.display.track) ?? null
                                            trackListViewModel: view.projectViewModelContext?.trackListViewModel ?? null
                                            editingItem: view.projectViewModelContext?.getClipViewItemFromDocumentItem(view.pianoRollPanelInterface?.editingClip ?? null) ?? null
                                            z: isEditingTrack ? 1 : 0
                                            active: modelData.display.overlayVisible || isEditingTrack
                                            bottomExpansion: view.bottomExpansion
                                        }
                                    }
                                }
                            }
                            Rectangle {
                                anchors.fill: parent
                                color: Qt.rgba(Theme.backgroundPrimaryColor.r, Theme.backgroundPrimaryColor.g, Theme.backgroundPrimaryColor.b, 0.5 * Theme.backgroundPrimaryColor.a)
                                visible: !view.pianoRollPanelInterface?.editingClip
                                clip: true
                                Item {
                                    anchors.fill: parent
                                    anchors.bottomMargin: view.bottomExpansion
                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        Label {
                                            text: qsTr("No clip selected")
                                            font.pixelSize: 20
                                            Layout.alignment: Qt.AlignHCenter
                                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                                        }
                                        Label {
                                            text: qsTr("Activate a clip to edit")
                                            Layout.alignment: Qt.AlignHCenter
                                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                                        }
                                    }
                                }
                            }
                            PianoRollScrollLayer {
                                anchors.fill: parent
                                timeViewModel: view.pianoRollPanelInterface?.timeViewModel ?? null
                                timeLayoutViewModel: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
                                clavierViewModel: view.pianoRollPanelInterface?.clavierViewModel ?? null
                                scrollBehaviorViewModel: view.pianoRollPanelInterface?.scrollBehaviorViewModel ?? null

                                bottomExpansion: view.bottomExpansion
                            }
                        }
                    }
                    Instantiator {
                        id: bottomAdditionalTrackRepeater
                        model: view.addOn?.bottomAdditionalTrackLoader.loadedComponents ?? []
                        delegate: Item {
                            id: bottomAdditionalTrackPanel
                            implicitHeight: layout.implicitHeight
                            required property string modelData
                            required property int index
                            SplitView.minimumHeight: 24
                            Component.onCompleted: () => {
                                SplitView.preferredHeight = implicitHeight
                            }
                            Action {
                                id: moveUpAction
                                enabled: bottomAdditionalTrackPanel.index !== 0
                                text: qsTr("Move Up")
                                icon.source: "image://fluent-system-icons/arrow_up"
                                onTriggered: view.addOn.bottomAdditionalTrackLoader.moveUp(bottomAdditionalTrackPanel.modelData)
                            }
                            Action {
                                id: moveDownAction
                                enabled: bottomAdditionalTrackPanel.index !== (view.addOn?.bottomAdditionalTrackLoader.loadedComponents.length ?? 0) - 1
                                text: qsTr("Move Down")
                                icon.source: "image://fluent-system-icons/arrow_down"
                                onTriggered: view.addOn.bottomAdditionalTrackLoader.moveDown(bottomAdditionalTrackPanel.modelData)
                            }
                            Action {
                                id: removeAction
                                text: qsTr("Remove")
                                icon.source: "image://fluent-system-icons/dismiss"
                                onTriggered: view.addOn.bottomAdditionalTrackLoader.removeItem(bottomAdditionalTrackPanel.modelData)
                            }
                            Rectangle {
                                anchors.fill: parent
                                color: Theme.backgroundPrimaryColor
                                opacity: 0.5
                            }
                            ColumnLayout {
                                id: layout
                                anchors.fill: parent
                                spacing: 0
                                readonly property Item header: T.Pane {
                                    implicitHeight: 25
                                    Layout.fillWidth: true
                                    background:Rectangle {
                                        color: Theme.backgroundPrimaryColor
                                    }
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.bottomMargin: 1
                                        anchors.leftMargin: 4
                                        anchors.rightMargin: 4
                                        IconLabel {
                                            Layout.fillHeight: true
                                            icon.height: 16
                                            icon.width: 16
                                            spacing: 2
                                            icon.source: layout.item?.ActionInstantiator.icon.source ?? ""
                                            icon.color: layout.item?.ActionInstantiator.icon.color.valid ? layout.item.ActionInstantiator.icon.color : Theme.foregroundPrimaryColor
                                            text: view.addOn?.additionalTrackLoader.componentName(bottomAdditionalTrackPanel.modelData) ?? ""
                                            color: Theme.foregroundPrimaryColor
                                            font: Theme.font
                                        }
                                        Item {
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                        }
                                        ToolButton {
                                            implicitWidth: 20
                                            implicitHeight: 20
                                            padding: 2
                                            display: AbstractButton.IconOnly
                                            action: moveUpAction
                                        }
                                        ToolButton {
                                            implicitWidth: 20
                                            implicitHeight: 20
                                            padding: 2
                                            display: AbstractButton.IconOnly
                                            action: moveDownAction
                                        }
                                        ToolButton {
                                            implicitWidth: 20
                                            implicitHeight: 20
                                            padding: 2
                                            display: AbstractButton.IconOnly
                                            action: removeAction
                                        }
                                    }
                                    Rectangle {
                                        width: parent.width
                                        anchors.bottom: parent.bottom
                                        implicitHeight: 1
                                        color: Theme.paneSeparatorColor
                                    }
                                }
                                readonly property Item item: view.addOn?.bottomAdditionalTrackLoader.loadedItems[bottomAdditionalTrackPanel.index] ?? null
                                data: [header, item]
                                Connections {
                                    target: layout.item
                                    function onHeightChanged() {
                                        layout.item.Layout.preferredHeight = layout.item.height
                                    }
                                }
                            }
                        }
                        onObjectAdded: (index, object) => {
                            noteAreaSplitView.insertItem(index + 1, object)
                        }
                        onObjectRemoved: (index, object) => {
                            noteAreaSplitView.removeItem(object)
                        }
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
