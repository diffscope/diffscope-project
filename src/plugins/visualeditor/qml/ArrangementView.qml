// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

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

    property Item selectionLayer: null
    property int selectionTimeCursorPosition: -1
    readonly property ArrangementClipEditLayer clipPane:
        contentLayers.items.find(item => item instanceof ArrangementClipEditLayer) ?? null
    readonly property Item timeCursorSource:
        foregroundLayers.items.find(item => item.visible && item.enabled && item.timeCursorActive === true) ?? null
    readonly property bool timeCursorBoundByEdit:
        selectionLayer !== null || (clipPane?.clipCursorActive ?? false)
        || timeline.dragging || timeline.adjustingLoop || timeCursorSource !== null

    onTimeCursorBoundByEditChanged: refreshCursorPosition()

    function updateSelectionCursorPosition() {
        if (!view.selectionLayer || !selectionPointerHandler.active)
            return
        const point = selectionPointerTracker.mapToItem(
            clipArea, selectionPointerHandler.point.position)
        view.selectionTimeCursorPosition = Math.max(0,
            timeManipulator.mapToPosition(point.x))
    }

    function beginSelection(layer: Item) {
        if (!layer)
            return
        view.selectionLayer = layer
        view.updateSelectionCursorPosition()
    }

    function endSelection(layer) {
        if (view.selectionLayer !== layer)
            return
        view.selectionLayer = null
        view.selectionTimeCursorPosition = -1
    }

    function refreshCursorPosition() {
        if (view.selectionLayer) {
            view.updateSelectionCursorPosition()
            return
        }
        if (view.timeCursorBoundByEdit || !view.arrangementPanelInterface?.timeLayoutViewModel)
            return
        if (!arrangementHoverHandler.enabled || !arrangementHoverHandler.hovered) {
            view.arrangementPanelInterface.timeLayoutViewModel.cursorPosition = -1
            return
        }
        const position = timeManipulator.mapToPosition(arrangementHoverHandler.point.position.x)
        view.arrangementPanelInterface.timeLayoutViewModel.cursorPosition =
            view.arrangementPanelInterface.tool === ArrangementPanelInterface.SelectTool
                ? Math.max(0, position)
                : timeManipulator.alignPosition(position, ScopicFlow.AO_Visible)
    }

    Binding {
        target: view.arrangementPanelInterface?.timeLayoutViewModel ?? null
        property: "cursorPosition"
        value: view.selectionTimeCursorPosition
        when: view.selectionLayer !== null
        restoreMode: Binding.RestoreNone
    }

    Binding {
        target: view.arrangementPanelInterface?.timeLayoutViewModel ?? null
        property: "cursorPosition"
        value: view.timeCursorSource?.timeCursorPosition ?? -1
        when: view.timeCursorSource !== null && view.selectionLayer === null
            && !(view.clipPane?.clipCursorActive ?? false)
            && !timeline.dragging && !timeline.adjustingLoop
        restoreMode: Binding.RestoreNone
    }

    Connections {
        target: view.clipPane

        function onSelectionStarted() {
            view.beginSelection(view.clipPane)
        }

        function onSelectionFinished() {
            view.endSelection(view.clipPane)
        }
    }

    Connections {
        target: view.arrangementPanelInterface
        function onToolChanged() {
            view.refreshCursorPosition()
        }
    }

    Connections {
        target: view.arrangementPanelInterface?.timeViewModel ?? null
        enabled: view.selectionLayer !== null
        function onStartChanged() {
            view.updateSelectionCursorPosition()
        }
    }

    Connections {
        target: view.arrangementPanelInterface?.timeLayoutViewModel ?? null
        enabled: view.selectionLayer !== null
        function onPixelDensityChanged() {
            view.updateSelectionCursorPosition()
        }
    }

    TimelineContextMenuHelper {
        timeline: view.timeline
        windowHandle: view.arrangementPanelInterface?.windowHandle ?? null
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
                EditorTimeline {
                    id: timeline
                    Layout.fillWidth: true
                    timeViewModel: view.arrangementPanelInterface?.timeViewModel ?? null
                    timeLayoutViewModel: view.arrangementPanelInterface?.timeLayoutViewModel ?? null
                    playbackViewModel: view.projectViewModelContext?.playbackViewModel ?? null
                    scrollBehaviorViewModel: view.arrangementPanelInterface?.scrollBehaviorViewModel ?? null
                    timelineInteractionController: view.arrangementPanelInterface?.timelineInteractionController ?? null
                    layerPoint: ViewLayer.ArrangementTimelineForeground
                    arrangementPanelInterface: view.arrangementPanelInterface
                    onSelectionStarted: view.beginSelection(timeline)
                    onSelectionFinished: view.endSelection(timeline)
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
                    ClipPaneBackground {
                        anchors.fill: parent
                        trackListViewModel: view.projectViewModelContext?.trackListViewModel ?? null
                        trackListLayoutViewModel: view.arrangementPanelInterface?.trackListLayoutViewModel ?? null
                        timeViewModel: view.arrangementPanelInterface?.timeViewModel ?? null
                        timeLayoutViewModel: view.arrangementPanelInterface?.timeLayoutViewModel ?? null
                    }
                    ViewLayerHost {
                        anchors.fill: parent
                        point: ViewLayer.ArrangementBackground
                        arrangementPanelInterface: view.arrangementPanelInterface
                    }
                    ViewLayerHost {
                        id: contentLayers
                        anchors.fill: parent
                        point: ViewLayer.ArrangementContent
                        arrangementPanelInterface: view.arrangementPanelInterface
                        pianoRollPanelInterface: view.addOn?.pianoRollPanelInterface ?? null
                    }
                    ViewLayerHost {
                        id: foregroundLayers
                        anchors.fill: parent
                        point: ViewLayer.ArrangementForeground
                        arrangementPanelInterface: view.arrangementPanelInterface
                    }
                    Item {
                        id: selectionOverlay
                        anchors.fill: parent
                        clip: true
                    }
                    Binding {
                        target: view.clipPane
                        property: "selectionOverlayParent"
                        value: selectionOverlay
                        when: view.clipPane !== null
                        restoreMode: Binding.RestoreNone
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
            Item {
                id: selectionPointerTracker
                anchors.fill: parent
                z: 1

                // A passive grab preserves pointer tracking during selection without taking the editor mouse grab.
                PointHandler {
                    id: selectionPointerHandler
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: undefined
                    onPointChanged: view.updateSelectionCursorPosition()
                }
            }
            HoverHandler {
                id: arrangementHoverHandler
                enabled: !(view.arrangementPanelInterface?.mouseTrackingDisabled)
                cursorShape: undefined
                readonly property TimeManipulator timeManipulator: TimeManipulator {
                    id: timeManipulator
                    target: parent
                    timeViewModel: view.arrangementPanelInterface?.timeViewModel ?? null
                    timeLayoutViewModel: view.arrangementPanelInterface?.timeLayoutViewModel ?? null
                }
                onHoveredChanged: view.refreshCursorPosition()
                onPointChanged: view.refreshCursorPosition()
                onEnabledChanged: view.refreshCursorPosition()
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
