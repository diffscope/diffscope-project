// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl as Impl
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views
import dev.sjimo.ScopicFlow.Internal as ScopicFlowInternal

import DiffScope.Core

QtObject {
    id: d
    required property QtObject addOn

    readonly property ProjectViewModelContext projectViewModelContext:
        addOn?.windowHandle.ProjectViewModelContext.context ?? null

    readonly property Component parameterTrackComponent: Item {
        id: control
        required property PianoRollPanelInterface contextObject

        readonly property ParameterEditorContext parameterContext:
            d.projectViewModelContext?.parameterEditorContext ?? null
        readonly property ParameterEditorRow currentRow: {
            const context = control.parameterContext
            if (!context)
                return null
            const rows = context.rows
            if (rows.length === 0)
                return null
            return rows[Math.max(0, Math.min(context.currentRowIndex, rows.length - 1))]
        }
        property int currentTool: ParameterEditorInteractionController.Pencil

        function moveCurrentRow(delta) {
            const context = control.parameterContext
            if (!context)
                return
            context.currentRowIndex = Math.max(0,
                Math.min(context.rows.length - 1, context.currentRowIndex + delta))
        }

        function applyTool(tool) {
            control.currentTool = tool
            for (let i = 0; i < parameterRowRepeater.count; ++i) {
                const row = parameterRowRepeater.itemAt(i)
                if (!row)
                    continue
                if (i === (control.parameterContext?.currentRowIndex ?? -1))
                    row.setTool(tool)
                else
                    row.updateTool(tool)
            }
        }

        readonly property Item toolBar: RowLayout {
            spacing: 5

            component ParameterComboBox: ComboBox {
                id: combo
                required property bool editing
                readonly property string selectedId: editing
                    ? (control.currentRow?.editingParameterId ?? "")
                    : (control.currentRow?.referenceParameterId ?? "")
                readonly property string selectedName: editing
                    ? (control.currentRow?.editingParameterDisplayName ?? qsTr("None"))
                    : (control.currentRow?.referenceParameterDisplayName ?? qsTr("None"))
                readonly property bool selectedWarning: selectedId.length > 0 && (editing
                    ? !(control.currentRow?.editingBinding?.supported ?? false)
                    : !(control.currentRow?.referenceDataBinding?.supported ?? false))

                implicitWidth: 132
                implicitHeight: 22
                model: control.parameterContext?.parameterModel ?? null
                textRole: "displayName"
                valueRole: "parameterId"
                displayText: currentIndex >= 0 ? currentText : selectedName

                Component.onCompleted: currentValue = Qt.binding(() => selectedId)
                onActivated: index => {
                    const row = control.currentRow
                    if (!row)
                        return
                    const value = valueAt(index)
                    if (editing)
                        row.editingParameterId = value
                    else
                        row.referenceParameterId = value
                }

                delegate: ItemDelegate {
                    id: delegateItem
                    required property string displayName
                    required property bool warning
                    required property int index
                    width: combo.width
                    height: combo.implicitHeight
                    padding: 2
                    leftPadding: 8
                    rightPadding: 8
                    highlighted: combo.highlightedIndex === index
                    hoverEnabled: combo.hoverEnabled
                    text: displayName

                    contentItem: RowLayout {
                        spacing: 4

                        Impl.IconImage {
                            Layout.preferredWidth: 14
                            Layout.preferredHeight: 14
                            visible: delegateItem.warning
                            source: delegateItem.warning
                                ? "image://fluent-system-icons/warning"
                                : ""
                            sourceSize: Qt.size(14, 14)
                            color: Theme.foregroundPrimaryColor
                        }
                        Label {
                            Layout.fillWidth: true
                            text: delegateItem.displayName
                            font: delegateItem.font
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                contentItem: Item {
                    implicitWidth: selectedContent.implicitWidth + 8
                    implicitHeight: selectedContent.implicitHeight

                    RowLayout {
                        id: selectedContent
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        spacing: 3

                        Impl.IconImage {
                            visible: combo.selectedWarning
                            source: "image://fluent-system-icons/warning"
                            sourceSize: Qt.size(14, 14)
                            color: Theme.foregroundPrimaryColor
                        }
                        Label {
                            Layout.fillWidth: true
                            text: combo.displayText
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            ToolButton {
                implicitWidth: 22
                implicitHeight: 22
                padding: 2
                text: qsTr("Add Parameter Row")
                icon.source: "image://fluent-system-icons/add"
                display: AbstractButton.IconOnly
                onClicked: {
                    if (control.parameterContext)
                        control.parameterContext.addRow()
                }
            }
            ToolButton {
                implicitWidth: 22
                implicitHeight: 22
                padding: 2
                text: qsTr("Remove Parameter Row")
                icon.source: "image://fluent-system-icons/subtract"
                display: AbstractButton.IconOnly
                visible: (control.parameterContext?.rows.length ?? 0) > 1
                onClicked: {
                    if (control.parameterContext)
                        control.parameterContext.removeRow(control.currentRow)
                }
            }
            Label {
                Layout.alignment: Qt.AlignVCenter
                visible: (control.parameterContext?.rows.length ?? 0) > 1
                text: ((control.parameterContext?.currentRowIndex ?? 0) + 1).toLocaleString()
                    + "/" + (control.parameterContext?.rows.length ?? 0).toLocaleString()
                ThemedItem.foregroundLevel: SVS.FL_Secondary
            }
            ToolButton {
                implicitWidth: 22
                implicitHeight: 22
                padding: 2
                text: qsTr("Previous Parameter Row")
                icon.source: "image://fluent-system-icons/chevron_up"
                display: AbstractButton.IconOnly
                visible: (control.parameterContext?.rows.length ?? 0) > 1
                enabled: (control.parameterContext?.currentRowIndex ?? 0) > 0
                onClicked: control.moveCurrentRow(-1)
            }
            ToolButton {
                implicitWidth: 22
                implicitHeight: 22
                padding: 2
                text: qsTr("Next Parameter Row")
                icon.source: "image://fluent-system-icons/chevron_down"
                display: AbstractButton.IconOnly
                visible: (control.parameterContext?.rows.length ?? 0) > 1
                enabled: (control.parameterContext?.currentRowIndex ?? 0)
                    < (control.parameterContext?.rows.length ?? 0) - 1
                onClicked: control.moveCurrentRow(1)
            }

            Label {
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Edit")
                ThemedItem.foregroundLevel: SVS.FL_Secondary
            }
            ParameterComboBox {
                editing: true
                DescriptiveText.toolTip: qsTr("Editing parameter")
                DescriptiveText.activated: hovered
            }
            ToolButton {
                implicitWidth: 22
                implicitHeight: 22
                padding: 2
                text: qsTr("Swap")
                icon.source: "image://fluent-system-icons/arrow_swap"
                display: AbstractButton.IconOnly
                onClicked: {
                    if (control.currentRow)
                        control.currentRow.swapParameters()
                }
            }
            Label {
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Ref")
                ThemedItem.foregroundLevel: SVS.FL_Secondary
            }
            ParameterComboBox {
                editing: false
                DescriptiveText.toolTip: qsTr("Reference parameter")
                DescriptiveText.activated: hovered
            }

            ToolBarContainerSeparator {
            }

            ToolButton {
                implicitHeight: 22
                padding: 3
                icon.source: "image://fluent-system-icons/adr"
                text: qsTr("Transform")
                ThemedItem.controlType: SVS.CT_Accent
                checkable: true
                checked: control.parameterContext?.transformEditing ?? false
                enabled: control.currentRow?.editingBinding?.available ?? false
                onClicked: {
                    if (control.parameterContext)
                        control.parameterContext.transformEditing = checked
                }
            }

            ToolBarContainerSeparator {
            }

            Repeater {
                model: [
                    { text: qsTr("Pencil"), icon: "edit", tool: ParameterEditorInteractionController.Pencil },
                    { text: qsTr("Line"), icon: "line", tool: ParameterEditorInteractionController.Line },
                    { text: qsTr("Fixing"), icon: "paint_brush_arrow_down", tool: ParameterEditorInteractionController.Brush },
                    { text: qsTr("Transform Tool"), icon: "pitch_transform", tool: ParameterEditorInteractionController.Transform },
                    { text: qsTr("Eraser"), icon: "eraser", tool: ParameterEditorInteractionController.Eraser },
                    { text: qsTr("Range Select"), icon: "cursor_text", tool: ParameterEditorInteractionController.FreeRangeSelect },
                    { text: qsTr("Pointer"), icon: "cursor", tool: ParameterEditorInteractionController.Pointer },
                    { text: qsTr("Pen"), icon: "calligraphy_pen", tool: ParameterEditorInteractionController.Pen },
                    { text: qsTr("Convert"), icon: "arrow_bidirectional_left_right", tool: ParameterEditorInteractionController.ConvertAnchor },
                    { text: qsTr("Anchors Select"), icon: "rectangle_landscape_dash", tool: ParameterEditorInteractionController.AnchorRubberBandSelect }
                ]
                delegate: ToolButton {
                    required property var modelData
                    implicitHeight: 22
                    padding: 3
                    text: modelData.text
                    icon.source: modelData.icon.length > 0
                        ? "image://fluent-system-icons/" + modelData.icon
                        : ""
                    display: modelData.icon.length > 0 ? AbstractButton.IconOnly : AbstractButton.TextOnly
                    checkable: true
                    checked: control.currentTool === modelData.tool
                    ButtonGroup.group: parameterToolGroup
                    enabled: control.currentRow?.editingBinding?.available ?? false
                    onClicked: control.applyTool(modelData.tool)
                }
            }

            IconLabel {
                visible: (control.currentRow?.editingBinding?.available ?? false)
                    && !(control.currentRow?.editingBinding?.supported ?? false)
                text: qsTr("Unsupported parameter")
                icon.source: "image://fluent-system-icons/warning"
                icon.color: Theme.warningColor
                color: Theme.warningColor
                Accessible.name: qsTr("Unsupported parameter")
            }

            Item {
                Layout.fillWidth: true
            }

            ButtonGroup {
                id: parameterToolGroup
                exclusive: true
            }
        }

        implicitHeight: 160
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Repeater {
                id: parameterRowRepeater
                model: control.parameterContext?.rows ?? []
                delegate: ParameterRow {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 0
                    rowContext: control.parameterContext.rows[index]
                    contextObject: control.contextObject
                    projectViewModelContext: d.projectViewModelContext
                }
            }
        }

        component ParameterRow: Item {
            id: parameterRow
            required property int index
            required property ParameterEditorRow rowContext
            required property PianoRollPanelInterface contextObject
            required property ProjectViewModelContext projectViewModelContext

            readonly property ParameterViewModelBinding editingBinding:
                rowContext?.editingBinding ?? null
            readonly property ParameterViewModelBinding referenceBinding:
                rowContext?.referenceDataBinding ?? null
            property ParameterAnchorViewModel anchorBeingEdited: null
            property bool anchorMoving: false
            property bool freeEditing: false
            property ParameterEditor hoveredParameterEditor: null
            property bool parameterHovered: false
            readonly property int hoveredParameterPosition:
                hoveredParameterEditor?.pointerTimePosition ?? 0
            readonly property double hoveredParameterValue:
                hoveredParameterEditor?.pointerTransformedValue ?? 0.0
            readonly property bool timeCursorEditing: anchorMoving || freeEditing
            readonly property bool timeCursorUnaligned: true
            readonly property bool timeCursorActive:
                timeCursorEditing
                || (parameterHovered
                    && !(contextObject?.mouseTrackingDisabled ?? true))
            readonly property int timeCursorPosition:
                (anchorMoving ? (anchorBeingEdited?.position ?? 0)
                    : freeEditing ? activeParameterEditor.freeEditPointerPosition
                    : hoveredParameterPosition)
                + (proxyTimeViewModel.clipViewModel?.position ?? 0)
                - (proxyTimeViewModel.clipViewModel?.clipStart ?? 0)
            readonly property bool transformEditing:
                control.parameterContext?.transformEditing ?? false
            readonly property ParameterEditor activeParameterEditor:
                transformEditing ? transformEditor : editingEditor
            readonly property var editingTrackViewModel:
                projectViewModelContext?.getTrackViewItemFromDocumentItem(
                    contextObject?.editingClip?.clipSequence?.track ?? null) ?? null
            readonly property color activeCurveColor: editingTrackViewModel?.color ?? Theme.accentColor
            readonly property color dimmedEditingColor: Qt.rgba(
                activeCurveColor.r, activeCurveColor.g, activeCurveColor.b,
                activeCurveColor.a * (2.0 / 3.0))
            readonly property color foregroundCurveColor: Theme.foregroundPrimaryColor
            readonly property color finalCurveColor: Theme.foregroundSecondaryColor
            readonly property color transformCurveColor: Qt.rgba(
                foregroundCurveColor.r, foregroundCurveColor.g, foregroundCurveColor.b,
                foregroundCurveColor.a * 0.8125)
            readonly property color transformDisplayCurveColor: Qt.rgba(
                transformCurveColor.r, transformCurveColor.g, transformCurveColor.b,
                transformCurveColor.a * 0.5)
            readonly property color referenceCurveColor: Qt.rgba(
                foregroundCurveColor.r, foregroundCurveColor.g, foregroundCurveColor.b,
                foregroundCurveColor.a * 0.375)
            readonly property color finalFillColor: Qt.rgba(
                activeCurveColor.r, activeCurveColor.g, activeCurveColor.b,
                activeCurveColor.a * 0.125)
            readonly property color referenceFillColor: Qt.rgba(
                referenceCurveColor.r, referenceCurveColor.g, referenceCurveColor.b,
                referenceCurveColor.a / 3.0)
            readonly property real activeFillSupplementAlpha:
                (activeCurveColor.a * 0.125) / (1.0 - activeCurveColor.a * 0.125)
            readonly property color activeFillSupplementColor: Qt.rgba(
                activeCurveColor.r, activeCurveColor.g, activeCurveColor.b,
                activeFillSupplementAlpha)
            readonly property color transparentCurveColor: Qt.rgba(
                foregroundCurveColor.r, foregroundCurveColor.g, foregroundCurveColor.b, 0.0)
            readonly property color defaultReferenceColor: Qt.rgba(
                foregroundCurveColor.r, foregroundCurveColor.g, foregroundCurveColor.b,
                foregroundCurveColor.a * 0.5)

            function resetController(controller) {
                if (!controller)
                    return
                controller.primaryItemInteraction = ParameterEditorInteractionController.None
                controller.secondaryItemInteraction = ParameterEditorInteractionController.None
                controller.primarySceneInteraction = ParameterEditorInteractionController.None
                controller.secondarySceneInteraction = ParameterEditorInteractionController.None
                controller.primarySelectInteraction = ParameterEditorInteractionController.None
                controller.secondarySelectInteraction = ParameterEditorInteractionController.None
            }

            function beginAnchorMove(editor, item) {
                if (editor !== editingEditor && editor !== transformEditor)
                    return
                anchorBeingEdited = item
                anchorMoving = true
            }

            function endAnchorMove(editor) {
                if (editor !== editingEditor && editor !== transformEditor)
                    return
                anchorMoving = false
                anchorBeingEdited = null
            }

            function beginFreeEdit(editor) {
                if (editor !== editingEditor && editor !== transformEditor)
                    return
                freeEditing = true
            }

            function endFreeEdit(editor) {
                if (editor !== editingEditor && editor !== transformEditor)
                    return
                freeEditing = false
            }

            function beginSelection(editor) {
                if (editor !== editingEditor && editor !== transformEditor)
                    return
                clearParameterHover()
                contextObject?.pianoRollView?.beginSelection(editor)
            }

            function endSelection(editor) {
                contextObject?.pianoRollView?.endSelection(editor)
            }

            function isActiveParameterEditor(editor): bool {
                return transformEditing ? editor === transformEditor
                                        : editor === editingEditor
            }

            function updateParameterHover(editor) {
                if (!isActiveParameterEditor(editor))
                    return
                hoveredParameterEditor = editor
                parameterHovered = true
            }

            function endParameterHover(editor) {
                if (isActiveParameterEditor(editor) && !editor.hasPointerHover()) {
                    parameterHovered = false
                    hoveredParameterEditor = null
                }
            }

            function clearParameterHover() {
                parameterHovered = false
                hoveredParameterEditor = null
            }

            function alternateTool(tool: int): int {
                switch (tool) {
                case ParameterEditorInteractionController.Pencil:
                    return ParameterEditorInteractionController.Eraser
                case ParameterEditorInteractionController.Line:
                case ParameterEditorInteractionController.Brush:
                    return ParameterEditorInteractionController.Eraser
                case ParameterEditorInteractionController.Eraser:
                    return ParameterEditorInteractionController.Pencil
                case ParameterEditorInteractionController.FreeRangeSelect:
                    return ParameterEditorInteractionController.Transform
                case ParameterEditorInteractionController.Transform:
                    return ParameterEditorInteractionController.AlternativeTransform
                case ParameterEditorInteractionController.Pointer:
                case ParameterEditorInteractionController.Pen:
                    return ParameterEditorInteractionController.ConvertAnchor
                case ParameterEditorInteractionController.AnchorRubberBandSelect:
                    return ParameterEditorInteractionController.AnchorTimeRangeSelect
                default:
                    return tool
                }
            }

            function setTool(tool: int) {
                updateTool(tool)
                switch (tool) {
                case ParameterEditorInteractionController.Pencil:
                case ParameterEditorInteractionController.Line:
                case ParameterEditorInteractionController.Brush:
                case ParameterEditorInteractionController.Transform:
                case ParameterEditorInteractionController.Eraser:
                case ParameterEditorInteractionController.FreeRangeSelect:
                    if (transformEditing)
                        editingBinding?.focusTransformFreeLayer()
                    else
                        editingBinding?.focusFreeLayer()
                    break
                case ParameterEditorInteractionController.Pointer:
                case ParameterEditorInteractionController.Pen:
                case ParameterEditorInteractionController.AnchorRubberBandSelect:
                case ParameterEditorInteractionController.ConvertAnchor:
                    if (transformEditing)
                        editingBinding?.focusTransformAnchorLayer()
                    else
                        editingBinding?.focusAnchorLayer()
                    break
                }
            }

            function updateTool(tool: int) {
                resetController(editingBinding?.interactionController)
                resetController(editingBinding?.transformInteractionController)
                const controller = transformEditing
                    ? editingBinding?.transformInteractionController
                    : editingBinding?.interactionController
                if (!controller)
                    return
                const alternate = alternateTool(tool)
                switch (tool) {
                case ParameterEditorInteractionController.Pencil:
                case ParameterEditorInteractionController.Line:
                case ParameterEditorInteractionController.Brush:
                case ParameterEditorInteractionController.Transform:
                case ParameterEditorInteractionController.Eraser:
                case ParameterEditorInteractionController.FreeRangeSelect:
                    controller.primaryItemInteraction = tool
                    controller.primarySceneInteraction = tool
                    controller.primarySelectInteraction = ParameterEditorInteractionController.FreeRangeSelect
                    controller.secondaryItemInteraction = alternate
                    controller.secondarySceneInteraction = alternate
                    controller.secondarySelectInteraction = tool === ParameterEditorInteractionController.Transform
                        ? ParameterEditorInteractionController.FreeRangeSelect : ParameterEditorInteractionController.Transform
                    break
                case ParameterEditorInteractionController.Pointer:
                case ParameterEditorInteractionController.Pen:
                case ParameterEditorInteractionController.AnchorRubberBandSelect:
                    controller.primaryItemInteraction = tool
                    controller.primarySceneInteraction = tool
                    controller.primarySelectInteraction = ParameterEditorInteractionController.AnchorRubberBandSelect
                    controller.secondaryItemInteraction = alternate
                    controller.secondarySceneInteraction = alternate
                    controller.secondarySelectInteraction = ParameterEditorInteractionController.AnchorTimeRangeSelect
                    break
                case ParameterEditorInteractionController.ConvertAnchor:
                    controller.primaryItemInteraction = tool
                    controller.primarySelectInteraction = ParameterEditorInteractionController.AnchorRubberBandSelect
                    controller.secondaryItemInteraction = alternate
                    controller.secondarySelectInteraction = ParameterEditorInteractionController.AnchorTimeRangeSelect
                    break
                }
            }

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 0
            clip: true

            ScopicFlowInternal.ClipMappedProxyTimeViewModel {
                id: proxyTimeViewModel
                timeViewModel: contextObject?.timeViewModel ?? null
                clipViewModel: projectViewModelContext?.getClipViewItemFromDocumentItem(
                    contextObject?.editingClip ?? null) ?? null
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.rightButton
            }

            ViewLayerHost {
                anchors.fill: parent
                point: ViewLayer.ParameterBackground
                pianoRollPanelInterface: contextObject
            }

            ScopicFlowInternal.ParameterEditorContent {
                id: referenceLayer
                anchors.fill: parent
                z: 0
                visible: parameterRow.referenceBinding?.available ?? false
                freeParameterViewModel: parameterRow.referenceBinding?.freeEdited ?? null
                anchorParameterViewModel: parameterRow.referenceBinding?.anchorEdited ?? null
                originalParameterViewModel: parameterRow.referenceBinding?.original ?? null
                freeTransformParameterViewModel: parameterRow.referenceBinding?.freeTransform ?? null
                anchorTransformParameterViewModel: parameterRow.referenceBinding?.anchorTransform ?? null
                timeViewModel: proxyTimeViewModel
                timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
                fillMode: parameterRow.referenceBinding?.interactionController?.fillMode
                    ?? ScopicFlowInternal.ParameterEditorContent.NoFill
                fillBaseline: parameterRow.referenceBinding?.interactionController?.fillBaseline ?? 0.0
                referenceVisible: false
                defaultValueEnabled: parameterRow.referenceBinding?.interactionController?.defaultValueEnabled ?? false
                defaultValue: parameterRow.referenceBinding?.interactionController?.defaultValue ?? 0.0
                originalAndDefaultCurveDisplayMode: ScopicFlowInternal.ParameterEditorContent.CurveSolid
                editLayer: ScopicFlowInternal.ParameterEditorContent.FinalLayer
                curveColor: parameterRow.referenceCurveColor
                dimmedCurveColor: parameterRow.referenceCurveColor
                fillColor: parameterRow.referenceFillColor
                dimmedFillColor: parameterRow.referenceFillColor
                accentColor: parameterRow.referenceCurveColor
                referenceColor: parameterRow.referenceCurveColor
                selectedAnchorColor: parameterRow.referenceCurveColor
            }

            ScopicFlowInternal.ParameterEditorContent {
                id: finalParameterLayer
                anchors.fill: parent
                z: 1
                visible: parameterRow.transformEditing && (parameterRow.editingBinding?.available ?? false)
                freeParameterViewModel: parameterRow.editingBinding?.freeEdited ?? null
                anchorParameterViewModel: parameterRow.editingBinding?.anchorEdited ?? null
                originalParameterViewModel: parameterRow.editingBinding?.original ?? null
                freeTransformParameterViewModel: parameterRow.editingBinding?.freeTransform ?? null
                anchorTransformParameterViewModel: parameterRow.editingBinding?.anchorTransform ?? null
                timeViewModel: proxyTimeViewModel
                timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
                fillMode: parameterRow.editingBinding?.interactionController?.fillMode
                    ?? ScopicFlowInternal.ParameterEditorContent.NoFill
                fillBaseline: parameterRow.editingBinding?.interactionController?.fillBaseline ?? 0.0
                referenceVisible: false
                defaultValueEnabled: parameterRow.editingBinding?.interactionController?.defaultValueEnabled ?? false
                defaultValue: parameterRow.editingBinding?.interactionController?.defaultValue ?? 0.0
                originalAndDefaultCurveDisplayMode: ScopicFlowInternal.ParameterEditorContent.CurveSolid
                editLayer: ScopicFlowInternal.ParameterEditorContent.FinalLayer
                curveColor: parameterRow.finalCurveColor
                dimmedCurveColor: parameterRow.finalCurveColor
                fillColor: parameterRow.finalFillColor
                dimmedFillColor: parameterRow.finalFillColor
                accentColor: parameterRow.finalCurveColor
                referenceColor: parameterRow.finalCurveColor
                selectedAnchorColor: parameterRow.finalCurveColor
            }

            ScopicFlowInternal.ParameterEditorContent {
                id: transformDisplayLayer
                anchors.fill: parent
                z: 1
                visible: !parameterRow.transformEditing && (parameterRow.editingBinding?.available ?? false)
                freeParameterViewModel: parameterRow.editingBinding?.freeTransform ?? null
                anchorParameterViewModel: parameterRow.editingBinding?.anchorTransform ?? null
                timeViewModel: proxyTimeViewModel
                timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
                fillMode: ScopicFlowInternal.ParameterEditorContent.NoFill
                referenceVisible: false
                defaultValueEnabled: true
                defaultValue: 0.5
                originalAndDefaultCurveDisplayMode: ScopicFlowInternal.ParameterEditorContent.CurveSolid
                editLayer: ScopicFlowInternal.ParameterEditorContent.FinalLayer
                curveColor: parameterRow.transformDisplayCurveColor
                dimmedCurveColor: parameterRow.transformDisplayCurveColor
                accentColor: parameterRow.transformDisplayCurveColor
                referenceColor: parameterRow.transformDisplayCurveColor
                selectedAnchorColor: parameterRow.transformDisplayCurveColor
            }

            ScopicFlowInternal.ParameterEditorContent {
                id: activeParameterFillLayer
                anchors.fill: parent
                z: 1
                visible: !parameterRow.transformEditing && (parameterRow.editingBinding?.available ?? false)
                freeParameterViewModel: parameterRow.editingBinding?.freeEdited ?? null
                anchorParameterViewModel: parameterRow.editingBinding?.anchorEdited ?? null
                originalParameterViewModel: parameterRow.editingBinding?.original ?? null
                freeTransformParameterViewModel: parameterRow.editingBinding?.freeTransform ?? null
                anchorTransformParameterViewModel: parameterRow.editingBinding?.anchorTransform ?? null
                timeViewModel: proxyTimeViewModel
                timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
                fillMode: parameterRow.editingBinding?.interactionController?.fillMode
                    ?? ScopicFlowInternal.ParameterEditorContent.NoFill
                fillBaseline: parameterRow.editingBinding?.interactionController?.fillBaseline ?? 0.0
                referenceVisible: false
                defaultValueEnabled: parameterRow.editingBinding?.interactionController?.defaultValueEnabled ?? false
                defaultValue: parameterRow.editingBinding?.interactionController?.defaultValue ?? 0.0
                originalAndDefaultCurveDisplayMode: ScopicFlowInternal.ParameterEditorContent.CurveHidden
                editLayer: ScopicFlowInternal.ParameterEditorContent.FinalLayer
                curveColor: parameterRow.transparentCurveColor
                dimmedCurveColor: parameterRow.transparentCurveColor
                fillColor: parameterRow.activeFillSupplementColor
                dimmedFillColor: parameterRow.activeFillSupplementColor
                accentColor: parameterRow.transparentCurveColor
                referenceColor: parameterRow.transparentCurveColor
                selectedAnchorColor: parameterRow.transparentCurveColor
            }

            ParameterEditor {
                id: editingEditor
                anchors.fill: parent
                z: 2
                visible: !parameterRow.transformEditing && (parameterRow.editingBinding?.available ?? false)
                freeParameterViewModel: parameterRow.editingBinding?.freeEdited ?? null
                anchorParameterViewModel: parameterRow.editingBinding?.anchorEdited ?? null
                originalParameterViewModel: parameterRow.editingBinding?.original ?? null
                freeTransformParameterViewModel: parameterRow.editingBinding?.freeTransform ?? null
                anchorTransformParameterViewModel: parameterRow.editingBinding?.anchorTransform ?? null
                freeParameterSelectionViewModel: parameterRow.editingBinding?.freeSelection ?? null
                anchorSelectionController: parameterRow.editingBinding?.anchorSelectionController ?? null
                interactionController: parameterRow.editingBinding?.interactionController ?? null
                timeViewModel: proxyTimeViewModel
                timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
                Theme.accentColor: parameterRow.activeCurveColor
                Theme.foregroundSecondaryColor: parameterRow.dimmedEditingColor
                SFPalette.scaleSecondaryColor: parameterRow.defaultReferenceColor
            }

            ParameterEditor {
                id: transformEditor
                anchors.fill: parent
                z: 2
                visible: parameterRow.transformEditing && (parameterRow.editingBinding?.available ?? false)
                freeParameterViewModel: parameterRow.editingBinding?.freeTransform ?? null
                anchorParameterViewModel: parameterRow.editingBinding?.anchorTransform ?? null
                freeParameterSelectionViewModel: parameterRow.editingBinding?.transformFreeSelection ?? null
                anchorSelectionController: parameterRow.editingBinding?.transformAnchorSelectionController ?? null
                interactionController: parameterRow.editingBinding?.transformInteractionController ?? null
                timeViewModel: proxyTimeViewModel
                timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
                Theme.accentColor: parameterRow.activeCurveColor
                Theme.foregroundPrimaryColor: parameterRow.transformCurveColor
                Theme.foregroundSecondaryColor: parameterRow.transformCurveColor
                SFPalette.scaleSecondaryColor: parameterRow.defaultReferenceColor
            }

            ViewLayerHost {
                anchors.fill: parent
                z: 2
                point: ViewLayer.ParameterForeground
                pianoRollPanelInterface: contextObject
            }

            Item {
                id: scale
                width: 64
                height: parent.height
                z: 3
                visible: parameterRow.editingBinding?.available ?? false
                readonly property var activeParameterInfo: parameterRow.transformEditing
                    ? parameterRow.editingBinding?.transformParameterInfo
                    : parameterRow.editingBinding?.parameterInfo
                readonly property bool valueIndicatorEditing:
                    parameterRow.anchorMoving || parameterRow.freeEditing
                readonly property double valueIndicatorSource: parameterRow.anchorMoving
                    ? (parameterRow.anchorBeingEdited?.value ?? 0.0)
                    : parameterRow.freeEditing
                        ? parameterRow.activeParameterEditor.freeEditPointerValue
                        : parameterRow.hoveredParameterValue
                readonly property double valueIndicatorPosition: {
                    if (!Number.isFinite(valueIndicatorSource))
                        return 0.0
                    return Math.max(0.0, Math.min(1.0, valueIndicatorSource))
                }
                readonly property string valueIndicatorDisplayString: {
                    const info = activeParameterInfo
                    if (info === undefined || info === null
                            || !Number.isFinite(valueIndicatorSource)) {
                        return ""
                    }
                    return info.invokeToDisplayString(valueIndicatorPosition)
                }

                Label {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: 3
                    text: scale.activeParameterInfo?.invokeToDisplayString(1.0) ?? ""
                }
                Label {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.margins: 3
                    text: scale.activeParameterInfo?.invokeToDisplayString(0.0) ?? ""
                }
                Rectangle {
                    width: 1
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    color: Theme.foregroundPrimaryColor
                }
                Rectangle {
                    id: valueIndicator
                    x: 3
                    y: (1.0 - scale.valueIndicatorPosition) * scale.height
                        - height / 2.0
                    width: valueIndicatorLabel.implicitWidth + 8
                    height: valueIndicatorLabel.implicitHeight + 4
                    radius: 2
                    visible: (parameterRow.editingBinding?.available ?? false)
                        && (scale.valueIndicatorEditing
                            || (!(contextObject?.mouseTrackingDisabled ?? true)
                                && parameterRow.parameterHovered))
                        && scale.valueIndicatorDisplayString.length > 0
                    color: Qt.rgba(
                        Theme.backgroundPrimaryColor.r,
                        Theme.backgroundPrimaryColor.g,
                        Theme.backgroundPrimaryColor.b,
                        Theme.backgroundPrimaryColor.a * 0.5)

                    Label {
                        id: valueIndicatorLabel
                        anchors.centerIn: parent
                        text: scale.valueIndicatorDisplayString
                    }
                }
            }

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                z: 4
                visible: parameterRow.index > 0
                color: Theme.splitterColor
            }

            RowLayout {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 4
                z: 4
                spacing: 4
                visible: (control.parameterContext?.rows.length ?? 0) > 1

                Rectangle {
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    Layout.alignment: Qt.AlignVCenter
                    radius: 4
                    color: Theme.foregroundPrimaryColor
                    opacity: parameterRow.index === (control.parameterContext?.currentRowIndex ?? -1) ? 1 : 0
                    Accessible.ignored: true
                }
                Label {
                    Layout.alignment: Qt.AlignVCenter
                    text: parameterRow.rowContext?.editingParameterDisplayName ?? ""
                    color: Theme.foregroundPrimaryColor
                }
            }

            Component.onCompleted: {
                parameterRow.updateTool(control.currentTool)
                contextObject?.pianoRollView?.registerTimeCursorSource(parameterRow)
            }
            Component.onDestruction: {
                parameterRow.endSelection(editingEditor)
                parameterRow.endSelection(transformEditor)
                contextObject?.pianoRollView?.unregisterTimeCursorSource(parameterRow)
            }
            Connections {
                target: parameterRow.editingBinding
                function onTargetChanged() {
                    parameterRow.endSelection(editingEditor)
                    parameterRow.endSelection(transformEditor)
                    parameterRow.clearParameterHover()
                    parameterRow.updateTool(control.currentTool)
                }
            }
            Connections {
                target: control.parameterContext
                function onTransformEditingChanged() {
                    parameterRow.endSelection(editingEditor)
                    parameterRow.endSelection(transformEditor)
                    parameterRow.clearParameterHover()
                    parameterRow.updateTool(control.currentTool)
                }
            }
            Connections {
                target: parameterRow.editingBinding?.interactionController ?? null
                function onFreeEditingStarted(editor, operation, position, value) {
                    parameterRow.beginFreeEdit(editor)
                }
                function onFreeEditingCommitted(editor) {
                    parameterRow.endFreeEdit(editor)
                }
                function onFreeEditingAborted(editor) {
                    parameterRow.endFreeEdit(editor)
                }
                function onFreeRangeSelectingStarted(editor) {
                    if (editor === editingEditor)
                        parameterRow.beginSelection(editor)
                }
                function onFreeRangeSelectingCommitted(editor) {
                    if (editor === editingEditor)
                        parameterRow.endSelection(editor)
                }
                function onFreeRangeSelectingAborted(editor) {
                    if (editor === editingEditor)
                        parameterRow.endSelection(editor)
                }
                function onAnchorRubberBandDraggingStarted(editor) {
                    if (editor === editingEditor)
                        parameterRow.beginSelection(editor)
                }
                function onAnchorRubberBandDraggingCommitted(editor) {
                    if (editor === editingEditor)
                        parameterRow.endSelection(editor)
                }
                function onAnchorRubberBandDraggingAborted(editor) {
                    if (editor === editingEditor)
                        parameterRow.endSelection(editor)
                }
                function onAnchorMovingStarted(editor, item) {
                    parameterRow.beginAnchorMove(editor, item)
                }
                function onAnchorMovingCommitted(editor) {
                    parameterRow.endAnchorMove(editor)
                }
                function onAnchorMovingAborted(editor) {
                    parameterRow.endAnchorMove(editor)
                }
                function onHoverEntered(editor, position, value) {
                    parameterRow.updateParameterHover(editor)
                }
                function onHoverMoved(editor, position, value) {
                    parameterRow.updateParameterHover(editor)
                }
                function onHoverExited(editor) {
                    parameterRow.endParameterHover(editor)
                }
                function onItemHoverEntered(editor, item) {
                    parameterRow.updateParameterHover(editor)
                }
                function onItemHoverExited(editor, item) {
                    parameterRow.endParameterHover(editor)
                }
            }
            Connections {
                target: parameterRow.editingBinding?.transformInteractionController ?? null
                function onFreeEditingStarted(editor, operation, position, value) {
                    parameterRow.beginFreeEdit(editor)
                }
                function onFreeEditingCommitted(editor) {
                    parameterRow.endFreeEdit(editor)
                }
                function onFreeEditingAborted(editor) {
                    parameterRow.endFreeEdit(editor)
                }
                function onFreeRangeSelectingStarted(editor) {
                    if (editor === transformEditor)
                        parameterRow.beginSelection(editor)
                }
                function onFreeRangeSelectingCommitted(editor) {
                    if (editor === transformEditor)
                        parameterRow.endSelection(editor)
                }
                function onFreeRangeSelectingAborted(editor) {
                    if (editor === transformEditor)
                        parameterRow.endSelection(editor)
                }
                function onAnchorRubberBandDraggingStarted(editor) {
                    if (editor === transformEditor)
                        parameterRow.beginSelection(editor)
                }
                function onAnchorRubberBandDraggingCommitted(editor) {
                    if (editor === transformEditor)
                        parameterRow.endSelection(editor)
                }
                function onAnchorRubberBandDraggingAborted(editor) {
                    if (editor === transformEditor)
                        parameterRow.endSelection(editor)
                }
                function onAnchorMovingStarted(editor, item) {
                    parameterRow.beginAnchorMove(editor, item)
                }
                function onAnchorMovingCommitted(editor) {
                    parameterRow.endAnchorMove(editor)
                }
                function onAnchorMovingAborted(editor) {
                    parameterRow.endAnchorMove(editor)
                }
                function onHoverEntered(editor, position, value) {
                    parameterRow.updateParameterHover(editor)
                }
                function onHoverMoved(editor, position, value) {
                    parameterRow.updateParameterHover(editor)
                }
                function onHoverExited(editor) {
                    parameterRow.endParameterHover(editor)
                }
                function onItemHoverEntered(editor, item) {
                    parameterRow.updateParameterHover(editor)
                }
                function onItemHoverExited(editor, item) {
                    parameterRow.endParameterHover(editor)
                }
            }
            Connections {
                target: contextObject
                function onMouseTrackingDisabledChanged() {
                    if (contextObject?.mouseTrackingDisabled ?? true)
                        parameterRow.clearParameterHover()
                }
            }
        }
    }
}
