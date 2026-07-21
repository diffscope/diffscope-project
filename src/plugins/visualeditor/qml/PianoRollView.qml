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
import dev.sjimo.ScopicFlow.Internal as ScopicFlowInternal

import DiffScope.Core
import DiffScope.DspxModel as DspxModel
import DiffScope.DspxModel.SelectionModel as DspxSelectionModel

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
    property ParameterAnchorViewModel pitchAnchorBeingEdited: null
    property int pitchFreeEditPosition: 0
    property double pitchFreeEditValue: 0.0
    property double pitchClavierCursorPosition: -1.0
    property var cursorPositionSelectionOwners: []

    readonly property bool cursorPositionsHiddenBySelection:
        cursorPositionSelectionOwners.length > 0
    readonly property bool timeCursorBoundByEdit:
        noteStartCursorBinding.when || noteEndCursorBinding.when
        || pitchAnchorCursorBinding.when || pitchFreeEditCursorBinding.when

    function mapClipPositionToTimeline(position: int, clipViewModel): int {
        return position + (clipViewModel?.position ?? 0)
            - (clipViewModel?.clipStart ?? 0)
    }

    function pitchPositionFromNormalizedValue(value: double): double {
        const normalizedValue = Math.max(0.0, Math.min(1.0, value))
        const parameterInfo = view.projectViewModelContext?.parameterEditorContext?.pitchBinding?.parameterInfo
        const centValue = parameterInfo?.invokeDenormalize(normalizedValue)
            ?? normalizedValue * 12800.0
        return centValue / 100.0
    }

    function updatePitchClavierCursorPosition(point) {
        if (view.cursorPositionsHiddenBySelection
                || !pitchEditor.visible || pitchEditor.height <= 0
                || !view.itemContainsPoint(pianoRollViewContainerSplitViewport, point)) {
            view.pitchClavierCursorPosition = -1.0
            return
        }
        const pitchPoint = pitchEditor.mapFromItem(noteArea, point)
        const normalizedValue = 1.0 - pitchPoint.y / pitchEditor.height
        view.pitchClavierCursorPosition =
            view.pitchPositionFromNormalizedValue(normalizedValue)
    }

    function itemContainsPoint(item, point): bool {
        if (!item?.visible)
            return false
        const localPoint = item.mapFromItem(noteArea, point)
        return localPoint.x >= 0 && localPoint.x < item.width
            && localPoint.y >= 0 && localPoint.y < item.height
    }

    function isParameterEditingAt(point): bool {
        if ((view.pianoRollPanelInterface?.pitchToolActive ?? false)
                && view.itemContainsPoint(pianoRollViewContainerSplitViewport, point))
            return true

        const loader = view.addOn?.bottomAdditionalTrackLoader
        const parameterTrackId =
            "org.diffscope.visualeditor.pianoRollPanel.additionalTracks.parameter"
        const index = loader?.loadedComponents.indexOf(parameterTrackId) ?? -1
        if (index < 0)
            return false

        const parameterTrack = loader.loadedItems[index]
        return view.itemContainsPoint(parameterTrack, point)
    }

    function hideCursorPositionsForSelection() {
        if (view.pianoRollPanelInterface?.timeLayoutViewModel)
            view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = -1
        view.pitchClavierCursorPosition = -1.0
        if (!(view.pianoRollPanelInterface?.pitchToolActive ?? false)
                && view.pianoRollPanelInterface?.clavierViewModel) {
            view.pianoRollPanelInterface.clavierViewModel.cursorPosition = -1
        }
    }

    function clearHoverCursorPositions() {
        if (!view.timeCursorBoundByEdit
                && view.pianoRollPanelInterface?.timeLayoutViewModel) {
            view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = -1
        }
        if (view.pianoRollPanelInterface?.pitchToolActive ?? false) {
            if (!pitchAnchorCursorBinding.when && !pitchFreeEditCursorBinding.when)
                view.pitchClavierCursorPosition = -1.0
        } else if (!noteClavierCursorBinding.when
                && view.pianoRollPanelInterface?.clavierViewModel) {
            view.pianoRollPanelInterface.clavierViewModel.cursorPosition = -1
        }
    }

    function updateCursorPositionsFromHover(point) {
        if (view.cursorPositionsHiddenBySelection) {
            view.hideCursorPositionsForSelection()
            return
        }

        const p = noteArea.mapToItem(pianoRollViewContainer, point)
        if (!view.timeCursorBoundByEdit) {
            if (p.x < 0) {
                view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition = -1
            } else {
                const position = cursorTimeManipulator.mapToPosition(p.x)
                view.pianoRollPanelInterface.timeLayoutViewModel.cursorPosition =
                    view.isParameterEditingAt(point)
                        ? position
                        : cursorTimeManipulator.alignPosition(
                              position, ScopicFlow.AO_Visible)
            }
        }

        if (view.pianoRollPanelInterface?.pitchToolActive ?? false) {
            view.updatePitchClavierCursorPosition(point)
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
        if (view.cursorPositionsHiddenBySelection) {
            view.hideCursorPositionsForSelection()
        } else if (pianoRollHoverHandler.enabled && pianoRollHoverHandler.hovered) {
            view.updateCursorPositionsFromHover(pianoRollHoverHandler.point.position)
        } else {
            view.clearHoverCursorPositions()
        }
    }

    function beginCursorPositionHidingSelection(owner) {
        if (!owner || view.cursorPositionSelectionOwners.indexOf(owner) >= 0)
            return
        const owners = view.cursorPositionSelectionOwners.slice()
        owners.push(owner)
        view.cursorPositionSelectionOwners = owners
        view.hideCursorPositionsForSelection()
    }

    function endCursorPositionHidingSelection(owner) {
        const index = view.cursorPositionSelectionOwners.indexOf(owner)
        if (index < 0)
            return
        const owners = view.cursorPositionSelectionOwners.slice()
        owners.splice(index, 1)
        view.cursorPositionSelectionOwners = owners
        view.refreshCursorPositions()
    }

    function endNoteEditing(noteEditLayer) {
        if (noteEditLayer !== view.noteEditLayerBeingEdited)
            return
        view.noteDrawing = false
        noteStartCursorBinding.when = false
        noteEndCursorBinding.when = false
        noteClavierCursorBinding.when = false
        view.noteBeingEdited = null
        view.noteEditLayerBeingEdited = null
        view.refreshCursorPositions()
    }

    Binding {
        id: noteStartCursorBinding
        target: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
        property: "cursorPosition"
        value: view.mapClipPositionToTimeline(
            view.noteBeingEdited?.position ?? 0,
            view.noteEditLayerBeingEdited?.clipViewModel ?? null)
        when: false
    }

    Binding {
        id: noteEndCursorBinding
        target: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
        property: "cursorPosition"
        value: view.mapClipPositionToTimeline(
            (view.noteBeingEdited?.position ?? 0)
                + (view.noteBeingEdited?.length ?? 0),
            view.noteEditLayerBeingEdited?.clipViewModel ?? null)
        when: false
    }

    Binding {
        id: noteClavierCursorBinding
        target: view.pianoRollPanelInterface?.clavierViewModel ?? null
        property: "cursorPosition"
        value: view.noteBeingEdited?.key ?? -1
        when: false
    }

    Binding {
        id: pitchAnchorCursorBinding
        target: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
        property: "cursorPosition"
        value: view.mapClipPositionToTimeline(
            view.pitchAnchorBeingEdited?.position ?? 0,
            pitchProxyTimeViewModel?.clipViewModel ?? null)
        when: false
    }

    Binding {
        id: pitchFreeEditCursorBinding
        target: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
        property: "cursorPosition"
        value: view.mapClipPositionToTimeline(
            view.pitchFreeEditPosition,
            pitchProxyTimeViewModel?.clipViewModel ?? null)
        when: false
    }

    Binding {
        target: view.pianoRollPanelInterface?.clavierViewModel ?? null
        property: "cursorPosition"
        value: view.cursorPositionsHiddenBySelection
            ? -1.0
            : pitchAnchorCursorBinding.when
                ? view.pitchPositionFromNormalizedValue(
                      view.pitchAnchorBeingEdited?.value ?? 0.0)
                : pitchFreeEditCursorBinding.when
                    ? view.pitchPositionFromNormalizedValue(
                          view.pitchFreeEditValue)
                    : view.pitchClavierCursorPosition
        when: view.pianoRollPanelInterface?.pitchToolActive ?? false
    }

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

        function onMovingStarted(noteEditLayer, item) {
            view.noteDrawing = false
            view.noteEditLayerBeingEdited = noteEditLayer
            view.noteBeingEdited = item
            noteStartCursorBinding.when = true
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
                noteStartCursorBinding.when = true
            else
                noteEndCursorBinding.when = true
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
            view.beginCursorPositionHidingSelection(noteEditLayer)
        }

        function onRubberBandDraggingCommitted(noteEditLayer) {
            view.endCursorPositionHidingSelection(noteEditLayer)
        }

        function onRubberBandDraggingAborted(noteEditLayer) {
            view.endCursorPositionHidingSelection(noteEditLayer)
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
            noteEndCursorBinding.when = true
            noteClavierCursorBinding.when = true
        }
    }

    Connections {
        target: view.projectViewModelContext?.parameterEditorContext?.pitchBinding?.interactionController ?? null

        function onFreeEditingStarted(editor, operation, position, value) {
            if (editor !== pitchEditor)
                return
            view.pitchFreeEditPosition = position
            view.pitchFreeEditValue = value
            pitchFreeEditCursorBinding.when = true
        }

        function onFreeEditingUpdated(editor, operation, position, value) {
            if (editor !== pitchEditor)
                return
            view.pitchFreeEditPosition = position
            view.pitchFreeEditValue = value
        }

        function onFreeEditingCommitted(editor) {
            if (editor !== pitchEditor)
                return
            pitchFreeEditCursorBinding.when = false
            view.refreshCursorPositions()
        }

        function onFreeEditingAborted(editor) {
            if (editor !== pitchEditor)
                return
            pitchFreeEditCursorBinding.when = false
            view.refreshCursorPositions()
        }

        function onFreeRangeSelectingStarted(editor) {
            if (editor === pitchEditor)
                view.beginCursorPositionHidingSelection(editor)
        }

        function onFreeRangeSelectingCommitted(editor) {
            if (editor === pitchEditor)
                view.endCursorPositionHidingSelection(editor)
        }

        function onFreeRangeSelectingAborted(editor) {
            if (editor === pitchEditor)
                view.endCursorPositionHidingSelection(editor)
        }

        function onAnchorRubberBandDraggingStarted(editor) {
            if (editor === pitchEditor)
                view.beginCursorPositionHidingSelection(editor)
        }

        function onAnchorRubberBandDraggingCommitted(editor) {
            if (editor === pitchEditor)
                view.endCursorPositionHidingSelection(editor)
        }

        function onAnchorRubberBandDraggingAborted(editor) {
            if (editor === pitchEditor)
                view.endCursorPositionHidingSelection(editor)
        }

        function onAnchorMovingStarted(editor, item) {
            if (editor !== pitchEditor)
                return
            view.pitchAnchorBeingEdited = item
            pitchAnchorCursorBinding.when = true
        }

        function onAnchorMovingCommitted(editor) {
            if (editor !== pitchEditor)
                return
            pitchAnchorCursorBinding.when = false
            view.pitchAnchorBeingEdited = null
            view.refreshCursorPositions()
        }

        function onAnchorMovingAborted(editor) {
            if (editor !== pitchEditor)
                return
            pitchAnchorCursorBinding.when = false
            view.pitchAnchorBeingEdited = null
            view.refreshCursorPositions()
        }
    }

    Connections {
        target: view.pianoRollPanelInterface
        function onToolChanged() {
            view.refreshCursorPositions()
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
                                        view.addOn.windowHandle.projectDocumentContext.document.selectionModel.select(null, DspxSelectionModel.SelectionModel.Select, DspxSelectionModel.SelectionModel.ST_Note, clip.notes);
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
                    showCentText: view.pianoRollPanelInterface?.pitchToolActive ?? false

                    bottomExpansion: view.bottomExpansion

                    Connections {
                        target: clavier.clavierInteractionController
                        function onHoverEntered(clavier_, key) {
                            if (clavier_ === clavier
                                    && !(view.pianoRollPanelInterface?.pitchToolActive ?? false)
                                    && !view.cursorPositionsHiddenBySelection) {
                                clavier.clavierViewModel.cursorPosition = key
                            } else if (clavier_ === clavier
                                    && view.cursorPositionsHiddenBySelection) {
                                view.hideCursorPositionsForSelection()
                            }
                        }

                        function onHoverExited(clavier_, key) {
                            if (clavier_ === clavier
                                    && !(view.pianoRollPanelInterface?.pitchToolActive ?? false)) {
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

                        function onRubberBandDraggingStarted(timeline_) {
                            if (timeline_ === timeline)
                                view.beginCursorPositionHidingSelection(timeline)
                        }

                        function onRubberBandDraggingCommitted(timeline_) {
                            if (timeline_ === timeline)
                                view.endCursorPositionHidingSelection(timeline)
                        }

                        function onRubberBandDraggingAborted(timeline_) {
                            if (timeline_ === timeline)
                                view.endCursorPositionHidingSelection(timeline)
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
                                enabled: !(view.pianoRollPanelInterface?.pitchToolActive ?? false)
                                opacity: (view.pianoRollPanelInterface?.pitchToolActive ?? false) ? 0.5 : 1.0
                                Repeater {
                                    model: DelegateModel {
                                        model: view.pianoRollPanelInterface?.trackOverlaySelectorModel ?? null
                                        delegate: NoteEditLayerSequence {
                                            required property int index
                                            required property var modelData
                                            readonly property bool isEditingTrack:view.pianoRollPanelInterface?.editingClip?.clipSequence?.track === modelData.display.track
                                            anchors.fill: parent
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
                            ScopicFlowInternal.ClipMappedProxyTimeViewModel {
                                id: pitchProxyTimeViewModel
                                timeViewModel: view.pianoRollPanelInterface?.timeViewModel ?? null
                                clipViewModel: view.projectViewModelContext?.getClipViewItemFromDocumentItem(
                                    view.pianoRollPanelInterface?.editingClip ?? null) ?? null
                            }
                            Item {
                                id: pitchEditorViewport
                                width: parent.width
                                height: pianoRollViewContainerSplitViewport.height
                                z: 2
                                clip: true

                                ParameterEditor {
                                    id: pitchEditor
                                    readonly property ParameterViewModelBinding binding:
                                        view.projectViewModelContext?.parameterEditorContext.pitchBinding ?? null
                                    readonly property real keyHeight:
                                        view.pianoRollPanelInterface?.clavierViewModel?.pixelDensity ?? 0
                                    readonly property real clavierStart:
                                        view.pianoRollPanelInterface?.clavierViewModel?.start ?? 0
                                    readonly property real key128CenterY:
                                        (clavierStart - 128.5) * keyHeight
                                    readonly property real key0CenterY:
                                        (clavierStart - 0.5) * keyHeight
                                    x: 0
                                    width: parent.width
                                    y: key128CenterY
                                    height: key0CenterY - key128CenterY
                                    visible: binding?.available ?? false
                                    enabled: view.pianoRollPanelInterface?.pitchToolActive ?? false
                                    opacity: enabled ? 1.0 : 0.5
                                    freeParameterViewModel: binding?.freeEdited ?? null
                                    anchorParameterViewModel: binding?.anchorEdited ?? null
                                    originalParameterViewModel: binding?.original ?? null
                                    freeTransformParameterViewModel: binding?.freeTransform ?? null
                                    anchorTransformParameterViewModel: binding?.anchorTransform ?? null
                                    freeParameterSelectionViewModel: binding?.freeSelection ?? null
                                    anchorSelectionController: binding?.anchorSelectionController ?? null
                                    interactionController: binding?.interactionController ?? null
                                    timeViewModel: pitchProxyTimeViewModel
                                    timeLayoutViewModel: view.pianoRollPanelInterface?.timeLayoutViewModel ?? null
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
