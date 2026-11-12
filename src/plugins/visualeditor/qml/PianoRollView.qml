// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T

import QActionKit

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

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

    property NoteViewModel noteBeingEdited: null
    property Item noteEditLayerBeingEdited: null
    property bool noteDrawing: false
    property bool noteStartCursorActive: false
    property bool noteEndCursorActive: false
    property Item selectionLayer: null
    property bool selectionUsesClavier: false
    property bool noteSelectionActive: false
    property int selectionTimeCursorPosition: -1
    property double selectionClavierCursorPosition: -1.0
    property ParameterAnchorViewModel pitchAnchorBeingEdited: null
    property bool pitchAnchorMoving: false
    property bool pitchFreeEditing: false
    property bool pitchHovered: false
    readonly property PianoRollPitchLayer pitchLayer:
        contentLayers.items.find(item => item instanceof PianoRollPitchLayer) ?? null
    readonly property ParameterEditor pitchEditor: pitchLayer?.editor ?? null
    readonly property int pitchHoverPosition: pitchEditor?.pointerTimePosition ?? 0
    property double unalignedClavierCursorPosition: -1.0
    property var timeCursorSources: []

    readonly property bool unalignedTimeCursorToolActive:
        (pianoRollPanelInterface?.pitchToolActive ?? false)
        || pianoRollPanelInterface?.tool === PianoRollPanelInterface.SelectTool
        || (selectionLayer !== null && selectionUsesClavier)
    readonly property bool unalignedClavierCursorActive:
        !noteSelectionActive
        && ((pianoRollPanelInterface?.pitchToolActive ?? false)
            || (selectionLayer !== null && selectionUsesClavier))
    readonly property bool pitchHoverCursorActive:
        (pitchEditor?.enabled ?? false) && pitchHovered
        && !(pianoRollPanelInterface?.mouseTrackingDisabled ?? true)
    readonly property Item activeTimeCursorSource: {
        let hoveredSource = null
        for (const source of timeCursorSources) {
            if (!source?.visible || !source.enabled || !source.timeCursorActive)
                continue
            if (source.timeCursorEditing)
                return source
            if (!hoveredSource)
                hoveredSource = source
        }
        return hoveredSource
    }
    readonly property bool timeCursorBoundByEdit:
        selectionLayer !== null || noteStartCursorActive || noteEndCursorActive
        || pitchAnchorMoving || pitchFreeEditing || pitchHoverCursorActive
        || activeTimeCursorSource !== null
    readonly property int editTimeCursorPosition: {
        if (selectionLayer)
            return selectionTimeCursorPosition
        if (noteStartCursorActive || noteEndCursorActive) {
            return mapClipPositionToTimeline(
                (noteBeingEdited?.position ?? 0)
                    + (noteEndCursorActive ? (noteBeingEdited?.length ?? 0) : 0),
                noteEditLayerBeingEdited?.clipViewModel ?? null)
        }
        if (pitchAnchorMoving || pitchFreeEditing) {
            return mapClipPositionToTimeline(
                pitchAnchorMoving ? (pitchAnchorBeingEdited?.position ?? 0)
                    : (pitchEditor?.freeEditPointerPosition ?? 0),
                view.pitchLayer?.clipViewModel ?? null)
        }
        if (activeTimeCursorSource)
            return activeTimeCursorSource.timeCursorPosition
        if (pitchHoverCursorActive)
            return mapClipPositionToTimeline(pitchHoverPosition, view.pitchLayer?.clipViewModel ?? null)
        return -1
    }

    onActiveTimeCursorSourceChanged: refreshCursorPositions()
    onSelectionLayerChanged: refreshCursorPositions()

    function mapClipPositionToTimeline(position: int, clipViewModel): int {
        return position + (clipViewModel?.position ?? 0)
            - (clipViewModel?.clipStart ?? 0)
    }

    function pitchPositionFromNormalizedValue(value: double): double {
        const normalizedValue = Math.max(0.0, Math.min(1.0, value))
        const parameterInfo = view.projectViewModelContext?.parameterEditorContext?.pitchBinding?.parameterInfo
        return parameterInfo?.invokeToDisplayValue(normalizedValue)
            ?? normalizedValue * 128.0
    }

    function updateUnalignedClavierCursorPosition(point) {
        if (((view.pianoRollPanelInterface?.pitchToolActive ?? false)
                && (!pitchEditor?.visible || pitchEditor.height <= 0))
                || !view.itemContainsPoint(pianoRollViewContainerSplitViewport, point)) {
            view.unalignedClavierCursorPosition = -1.0
            return
        }
        const p = noteArea.mapToItem(pianoRollViewContainer, point)
        view.unalignedClavierCursorPosition = Math.max(0.0, Math.min(128.0,
            cursorClavierManipulator.mapToPosition(p.y) - 0.5))
    }

    function itemContainsPoint(item, point): bool {
        if (!item?.visible)
            return false
        const localPoint = item.mapFromItem(noteArea, point)
        return localPoint.x >= 0 && localPoint.x < item.width
            && localPoint.y >= 0 && localPoint.y < item.height
    }

    function clearHoverCursorPositions() {
        if (view.selectionLayer)
            return
        if (!view.timeCursorBoundByEdit
                && view.pianoRollPanelInterface?.timeLayoutViewModel) {
            view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = -1
        }
        if (view.unalignedClavierCursorActive) {
            if (!view.pitchAnchorMoving && !view.pitchFreeEditing)
                view.unalignedClavierCursorPosition = -1.0
        } else if (!noteClavierCursorBinding.when
                && view.pianoRollPanelInterface?.clavierViewModel) {
            view.pianoRollPanelInterface.clavierViewModel.cursorPosition = -1
        }
    }

    function updateCursorPositionsFromHover(point) {
        if (view.selectionLayer) {
            view.updateSelectionCursorPositions()
            return
        }
        const p = noteArea.mapToItem(pianoRollViewContainer, point)
        if (!view.timeCursorBoundByEdit) {
            if (p.x < 0) {
                view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = -1
            } else {
                const position = cursorTimeManipulator.mapToPosition(p.x)
                view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition =
                    view.usesUnalignedTimeCursorPosition(point)
                        ? position
                        : cursorTimeManipulator.alignPosition(
                            position, ScopicFlow.AO_Visible)
            }
        }

        if (view.unalignedClavierCursorActive) {
            view.updateUnalignedClavierCursorPosition(point)
        } else if (!noteClavierCursorBinding.when) {
            if (p.y < 0) {
                view.pianoRollPanelInterface.clavierViewModel.cursorPosition = -1
            } else {
                let key = Math.floor(cursorClavierManipulator.mapToPosition(p.y))
                if (key >= 128)
                    key = -1
                view.pianoRollPanelInterface.clavierViewModel.cursorPosition = key
            }
        }
    }

    function refreshCursorPositions() {
        if (view.selectionLayer) {
            view.updateSelectionCursorPositions()
        } else if (pianoRollHoverHandler.enabled && pianoRollHoverHandler.hovered) {
            view.updateCursorPositionsFromHover(pianoRollHoverHandler.point.position)
        } else {
            view.clearHoverCursorPositions()
        }
    }

    function updateSelectionCursorPositions() {
        if (!view.selectionLayer || !selectionPointerHandler.active)
            return
        const p = selectionPointerTracker.mapToItem(
            pianoRollViewContainer, selectionPointerHandler.point.position)
        view.selectionTimeCursorPosition = Math.max(0,
            cursorTimeManipulator.mapToPosition(p.x))
        const clavierPosition = cursorClavierManipulator.mapToPosition(p.y)
        view.selectionClavierCursorPosition = view.selectionUsesClavier
            ? view.noteSelectionActive
                ? Math.max(0, Math.min(127, Math.floor(clavierPosition)))
                : Math.max(0.0, Math.min(128.0, clavierPosition - 0.5))
            : -1.0
    }

    function beginSelection(layer: Item, selectsNotes) {
        if (!layer)
            return
        let usesClavier = false
        for (let item = layer; item; item = item.parent) {
            if (item === pianoRollViewContainerSplitViewport) {
                usesClavier = true
                break
            }
        }
        view.selectionUsesClavier = usesClavier
        view.noteSelectionActive = selectsNotes === true
        view.selectionLayer = layer
        view.updateSelectionCursorPositions()
    }

    function endSelection(layer) {
        if (view.selectionLayer !== layer)
            return
        view.selectionTimeCursorPosition = -1
        view.selectionClavierCursorPosition = -1.0
        view.selectionUsesClavier = false
        view.noteSelectionActive = false
        view.selectionLayer = null
    }

    function beginPitchHover(editor) {
        if (!pitchEditor || editor !== pitchEditor
                || !pitchEditor.enabled
                || (view.pianoRollPanelInterface?.mouseTrackingDisabled ?? true))
            return
        view.pitchHovered = true
    }

    function endPitchHover(editor) {
        if (!pitchEditor || editor !== pitchEditor || editor.hasPointerHover())
            return
        view.pitchHovered = false
        view.refreshCursorPositions()
    }

    // Registers a surface with cursor activity, edit state, alignment policy, and a position in timeline ticks.
    function registerTimeCursorSource(source) {
        if (!source || view.timeCursorSources.indexOf(source) >= 0)
            return
        const sources = view.timeCursorSources.slice()
        sources.push(source)
        view.timeCursorSources = sources
        view.refreshCursorPositions()
    }

    function unregisterTimeCursorSource(source) {
        const index = view.timeCursorSources.indexOf(source)
        if (index < 0)
            return
        const sources = view.timeCursorSources.slice()
        sources.splice(index, 1)
        view.timeCursorSources = sources
        view.refreshCursorPositions()
    }

    function usesUnalignedTimeCursorPosition(point): bool {
        if (view.unalignedTimeCursorToolActive)
            return true
        for (const source of view.timeCursorSources) {
            if (source?.enabled && source.timeCursorUnaligned
                    && view.itemContainsPoint(source, point))
                return true
        }
        return false
    }

    function endNoteEditing(noteEditLayer) {
        if (noteEditLayer !== view.noteEditLayerBeingEdited)
            return
        view.noteDrawing = false
        view.noteStartCursorActive = false
        view.noteEndCursorActive = false
        noteClavierCursorBinding.when = false
        view.noteBeingEdited = null
        view.noteEditLayerBeingEdited = null
        view.refreshCursorPositions()
    }

    Binding {
        target: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
        property: "cursorPosition"
        value: view.editTimeCursorPosition
        when: view.timeCursorBoundByEdit
        restoreMode: Binding.RestoreNone
    }

    Binding {
        id: noteClavierCursorBinding
        target: view.pianoRollPanelInterface?.clavierViewModel ?? null
        property: "cursorPosition"
        value: view.noteBeingEdited?.key ?? -1
        when: false
        restoreMode: Binding.RestoreNone
    }

    Binding {
        target: view.pianoRollPanelInterface?.clavierViewModel ?? null
        property: "cursorPosition"
        value: view.selectionClavierCursorPosition
        when: view.selectionLayer !== null
        restoreMode: Binding.RestoreNone
    }

    Binding {
        target: view.pianoRollPanelInterface?.clavierViewModel ?? null
        property: "cursorPosition"
        value: view.pitchAnchorMoving
            ? view.pitchPositionFromNormalizedValue(
                  view.pitchAnchorBeingEdited?.value ?? 0.0)
            : view.pitchFreeEditing
                ? view.pitchPositionFromNormalizedValue(
                      pitchEditor?.freeEditPointerValue ?? 0.0)
                : view.unalignedClavierCursorPosition
        when: view.unalignedClavierCursorActive && view.selectionLayer === null
        restoreMode: Binding.RestoreNone
    }

    TimelineContextMenuHelper {
        timeline: view.timeline
        windowHandle: view.pianoRollPanelInterface?.windowHandle ?? null
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

        function onMovingStarted(noteEditLayer, item) {
            view.noteDrawing = false
            view.noteEditLayerBeingEdited = noteEditLayer
            view.noteBeingEdited = item
            view.noteStartCursorActive = true
            noteClavierCursorBinding.when = true
        }

        function onMovingCommitted(noteEditLayer) {
            view.endNoteEditing(noteEditLayer)
        }

        function onMovingAborted(noteEditLayer) {
            view.endNoteEditing(noteEditLayer)
        }

        function onAdjustLengthStarted(noteEditLayer, item, edge) {
            view.noteDrawing = false
            view.noteEditLayerBeingEdited = noteEditLayer
            view.noteBeingEdited = item
            noteClavierCursorBinding.when = true
            if (edge === NoteEditLayerInteractionController.LeftEdge)
                view.noteStartCursorActive = true
            else
                view.noteEndCursorActive = true
        }

        function onAdjustLengthCommitted(noteEditLayer) {
            view.endNoteEditing(noteEditLayer)
        }

        function onAdjustLengthAborted(noteEditLayer) {
            view.endNoteEditing(noteEditLayer)
        }

        function onDrawingStarted(noteEditLayer) {
            view.noteDrawing = true
            view.noteEditLayerBeingEdited = noteEditLayer
            view.noteBeingEdited = null
        }

        function onDrawingCommitted(noteEditLayer) {
            view.endNoteEditing(noteEditLayer)
        }

        function onDrawingAborted(noteEditLayer) {
            view.endNoteEditing(noteEditLayer)
        }

        function onRubberBandDraggingStarted(noteEditLayer) {
            view.beginSelection(noteEditLayer, true)
        }

        function onRubberBandDraggingCommitted(noteEditLayer) {
            view.endSelection(noteEditLayer)
        }

        function onRubberBandDraggingAborted(noteEditLayer) {
            view.endSelection(noteEditLayer)
        }

        function onContextMenuRequested() {
            noteSceneContextMenu.popup()
        }

        function onItemContextMenuRequested() {
            noteItemContextMenu.popup()
        }
    }

    Connections {
        target: view.noteEditLayerBeingEdited?.noteSequenceViewModel ?? null
        enabled: view.noteDrawing

        function onItemInserted(item) {
            if (!view.noteDrawing || view.noteBeingEdited)
                return
            view.noteBeingEdited = item
            view.noteEndCursorActive = true
            noteClavierCursorBinding.when = true
        }
    }

    Connections {
        target: view.projectViewModelContext?.parameterEditorContext?.pitchBinding?.interactionController ?? null

        function onFreeEditingStarted(editor, operation, position, value) {
            if (editor !== pitchEditor)
                return
            view.pitchFreeEditing = true
        }

        function onFreeEditingCommitted(editor) {
            if (editor !== pitchEditor)
                return
            view.pitchFreeEditing = false
            view.refreshCursorPositions()
        }

        function onFreeEditingAborted(editor) {
            if (editor !== pitchEditor)
                return
            view.pitchFreeEditing = false
            view.refreshCursorPositions()
        }

        function onFreeRangeSelectingStarted(editor) {
            if (editor === pitchEditor) {
                view.pitchHovered = false
                view.beginSelection(editor)
            }
        }

        function onFreeRangeSelectingCommitted(editor) {
            if (editor === pitchEditor)
                view.endSelection(editor)
        }

        function onFreeRangeSelectingAborted(editor) {
            if (editor === pitchEditor)
                view.endSelection(editor)
        }

        function onAnchorRubberBandDraggingStarted(editor) {
            if (editor === pitchEditor) {
                view.pitchHovered = false
                view.beginSelection(editor)
            }
        }

        function onAnchorRubberBandDraggingCommitted(editor) {
            if (editor === pitchEditor)
                view.endSelection(editor)
        }

        function onAnchorRubberBandDraggingAborted(editor) {
            if (editor === pitchEditor)
                view.endSelection(editor)
        }

        function onAnchorMovingStarted(editor, item) {
            if (editor !== pitchEditor)
                return
            view.pitchAnchorBeingEdited = item
            view.pitchAnchorMoving = true
        }

        function onAnchorMovingCommitted(editor) {
            if (editor !== pitchEditor)
                return
            view.pitchAnchorMoving = false
            view.pitchAnchorBeingEdited = null
            view.refreshCursorPositions()
        }

        function onAnchorMovingAborted(editor) {
            if (editor !== pitchEditor)
                return
            view.pitchAnchorMoving = false
            view.pitchAnchorBeingEdited = null
            view.refreshCursorPositions()
        }

        function onHoverEntered(editor, position, value) {
            view.beginPitchHover(editor)
        }

        function onHoverMoved(editor, position, value) {
            view.beginPitchHover(editor)
        }

        function onHoverExited(editor) {
            view.endPitchHover(editor)
        }

        function onItemHoverEntered(editor, item) {
            view.beginPitchHover(editor)
        }

        function onItemHoverExited(editor, item) {
            view.endPitchHover(editor)
        }
    }

    Connections {
        target: view.pianoRollPanelInterface
        function onToolChanged() {
            if (!(view.pianoRollPanelInterface?.pitchToolActive ?? false))
                view.pitchHovered = false
            view.refreshCursorPositions()
        }
        function onMouseTrackingDisabledChanged() {
            if (view.pianoRollPanelInterface?.mouseTrackingDisabled ?? true)
                view.pitchHovered = false
            view.refreshCursorPositions()
        }
    }

    Connections {
        target: view.pianoRollPanelInterface?.timeViewModel ?? null
        enabled: view.selectionLayer !== null
        function onStartChanged() {
            view.updateSelectionCursorPositions()
        }
    }

    Connections {
        target: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
        enabled: view.selectionLayer !== null
        function onPixelDensityChanged() {
            view.updateSelectionCursorPositions()
        }
    }

    Connections {
        target: view.pianoRollPanelInterface?.clavierViewModel ?? null
        enabled: view.selectionLayer !== null
        function onStartChanged() {
            view.updateSelectionCursorPositions()
        }
        function onPixelDensityChanged() {
            view.updateSelectionCursorPositions()
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
                                    enabled: (trackRow.modelData.display.track?.clips.singingClipCount ?? 0) > 0
                                    onClicked: view.addOn.editTrack(trackRow.modelData.display.track)
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
                    showCentText: view.unalignedClavierCursorActive

                    bottomExpansion: view.bottomExpansion

                    ViewLayerHost {
                        anchors.fill: parent
                        point: ViewLayer.PianoRollKeyboardForeground
                        pianoRollPanelInterface: view.pianoRollPanelInterface
                    }

                    Connections {
                        target: clavier.clavierInteractionController
                        function onHoverEntered(clavier_, key) {
                            if (clavier_ === clavier
                                    && !view.unalignedClavierCursorActive
                                    && !view.selectionLayer) {
                                clavier.clavierViewModel.cursorPosition = key
                            }
                        }

                        function onHoverExited(clavier_, key) {
                            if (clavier_ === clavier
                                    && !view.unalignedClavierCursorActive
                                    && !view.selectionLayer) {
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
                EditorTimeline {
                    id: timeline
                    Layout.fillWidth: true
                    timeViewModel: view.pianoRollPanelInterface?.timeViewModel ?? null
                    timeLayoutViewModel: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
                    playbackViewModel: view.projectViewModelContext?.playbackViewModel ?? null
                    scrollBehaviorViewModel: view.pianoRollPanelInterface?.scrollBehaviorViewModel ?? null
                    timelineInteractionController: view.pianoRollPanelInterface?.timelineInteractionController ?? null
                    layerPoint: ViewLayer.PianoRollTimelineForeground
                    pianoRollPanelInterface: view.pianoRollPanelInterface
                    onSelectionStarted: view.beginSelection(timeline)
                    onSelectionFinished: view.endSelection(timeline)
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
                            ViewLayerHost {
                                anchors.fill: parent
                                point: ViewLayer.PianoRollBackground
                                pianoRollPanelInterface: view.pianoRollPanelInterface
                            }
                            ViewLayerHost {
                                id: contentLayers
                                anchors.fill: parent
                                point: ViewLayer.PianoRollContent
                                pianoRollPanelInterface: view.pianoRollPanelInterface
                                visibleRect: Qt.rect(0, 0, width, pianoRollViewContainerSplitViewport.height)
                            }
                            Rectangle {
                                id: noClipMessageLayer
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
                            ViewLayerHost {
                                width: parent.width
                                height: pianoRollViewContainerSplitViewport.height
                                point: ViewLayer.PianoRollForeground
                                pianoRollPanelInterface: view.pianoRollPanelInterface
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
                                opacity: EditorPreference.bottomPanelOpacity
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
                                            spacing: 2
                                            icon.source: layout.item?.ActionInstantiator.icon.source ?? ""
                                            icon.color: layout.item?.ActionInstantiator.icon.color.valid ? layout.item.ActionInstantiator.icon.color : Theme.foregroundPrimaryColor
                                            text: view.addOn?.additionalTrackLoader.componentName(bottomAdditionalTrackPanel.modelData) ?? ""
                                        }
                                        StackLayout {
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            data: [layout.item?.toolBar ?? null]
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
            Item {
                id: selectionPointerTracker
                anchors.fill: parent
                z: 1

                // A passive grab preserves pointer tracking during selection without taking the editor mouse grab.
                PointHandler {
                    id: selectionPointerHandler
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: undefined
                    onPointChanged: view.updateSelectionCursorPositions()
                }
            }
            HoverHandler {
                id: pianoRollHoverHandler
                enabled: !(view.pianoRollPanelInterface?.mouseTrackingDisabled)
                cursorShape: undefined
                readonly property TimeManipulator timeManipulator: TimeManipulator {
                    id: cursorTimeManipulator
                    target: parent
                    timeViewModel: view.pianoRollPanelInterface?.timeViewModel ?? null
                    timeLayoutViewModel: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
                }
                readonly property ClavierManipulator clavierManipulator: ClavierManipulator {
                    id: cursorClavierManipulator
                    target: parent
                    clavierViewModel: view.pianoRollPanelInterface?.clavierViewModel ?? null
                }
                onHoveredChanged: () => {
                    if (!hovered)
                        view.clearHoverCursorPositions()
                }
                onPointChanged: () =>
                    view.updateCursorPositionsFromHover(point.position)
                onEnabledChanged: {
                    if (!enabled)
                        view.clearHoverCursorPositions()
                }
            }
        }
    }
}
