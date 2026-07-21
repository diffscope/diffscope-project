pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
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
        readonly property ParameterViewModelBinding editingBinding:
            parameterContext?.editingBinding ?? null
        readonly property ParameterViewModelBinding referenceBinding:
            parameterContext?.referenceDataBinding ?? null
        property int currentTool: ParameterEditorInteractionController.Pencil
        property ParameterAnchorViewModel anchorBeingEdited: null
        property int freeEditPosition: 0
        property double freeEditValue: 0.0
        property bool parameterHovered: false
        property double hoveredParameterValue: 0.0
        readonly property bool transformEditing: parameterContext?.transformEditing ?? false
        readonly property var editingTrackViewModel:
            d.projectViewModelContext?.getTrackViewItemFromDocumentItem(
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
            anchorCursorBinding.when = true
        }

        function endAnchorMove(editor) {
            if (editor !== editingEditor && editor !== transformEditor)
                return
            anchorCursorBinding.when = false
            anchorBeingEdited = null
        }

        function beginFreeEdit(editor, position, value) {
            if (editor !== editingEditor && editor !== transformEditor)
                return
            freeEditPosition = position
            freeEditValue = value
            freeEditCursorBinding.when = true
        }

        function updateFreeEdit(editor, position, value) {
            if (editor !== editingEditor && editor !== transformEditor)
                return
            freeEditPosition = position
            freeEditValue = value
        }

        function endFreeEdit(editor) {
            if (editor !== editingEditor && editor !== transformEditor)
                return
            freeEditCursorBinding.when = false
        }

        function beginCursorPositionHidingSelection(editor) {
            const pianoRollView = contextObject?.pianoRollView
            if (pianoRollView) {
                pianoRollView.beginCursorPositionHidingSelection(editor)
            } else {
                if (contextObject?.timeLayoutViewModel)
                    contextObject.timeLayoutViewModel.cursorPosition = -1
                if (contextObject?.clavierViewModel)
                    contextObject.clavierViewModel.cursorPosition = -1
            }
        }

        function endCursorPositionHidingSelection(editor) {
            contextObject?.pianoRollView?.endCursorPositionHidingSelection(editor)
        }

        function isActiveParameterEditor(editor): bool {
            return transformEditing ? editor === transformEditor
                                    : editor === editingEditor
        }

        function updateParameterHover(editor, value) {
            if (!isActiveParameterEditor(editor))
                return
            hoveredParameterValue = value
            parameterHovered = true
        }

        function endParameterHover(editor) {
            if (isActiveParameterEditor(editor))
                parameterHovered = false
        }

        function alternateTool(tool: int): int {
            switch (tool) {
            case ParameterEditorInteractionController.Pencil:
                return ParameterEditorInteractionController.Eraser
            case ParameterEditorInteractionController.Eraser:
                return ParameterEditorInteractionController.Pencil
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
            currentTool = tool
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
            case ParameterEditorInteractionController.Eraser:
            case ParameterEditorInteractionController.FreeRangeSelect:
            case ParameterEditorInteractionController.Pen:
                if (tool === ParameterEditorInteractionController.Pencil
                        || tool === ParameterEditorInteractionController.Eraser
                        || tool === ParameterEditorInteractionController.FreeRangeSelect) {
                    if (transformEditing)
                        editingBinding.focusTransformFreeLayer()
                    else
                        editingBinding.focusFreeLayer()
                } else {
                    if (transformEditing)
                        editingBinding.focusTransformAnchorLayer()
                    else
                        editingBinding.focusAnchorLayer()
                }
                controller.primaryItemInteraction = tool
                controller.primarySceneInteraction = tool
                controller.secondaryItemInteraction = alternate
                controller.secondarySceneInteraction = alternate
                break
            case ParameterEditorInteractionController.Pointer:
            case ParameterEditorInteractionController.AnchorRubberBandSelect:
                if (transformEditing)
                    editingBinding.focusTransformAnchorLayer()
                else
                    editingBinding.focusAnchorLayer()
                controller.primaryItemInteraction = tool
                controller.primarySceneInteraction = tool
                controller.primarySelectInteraction = tool
                controller.secondaryItemInteraction = alternate
                controller.secondarySceneInteraction = alternate
                controller.secondarySelectInteraction = alternate
                break
            case ParameterEditorInteractionController.ConvertAnchor:
                if (transformEditing)
                    editingBinding.focusTransformAnchorLayer()
                else
                    editingBinding.focusAnchorLayer()
                controller.primaryItemInteraction = tool
                controller.secondaryItemInteraction = alternate
                break
            }
        }

        readonly property Item toolBar: RowLayout {
            spacing: 5

            component ParameterComboBox: ComboBox {
                id: combo
                required property bool editing
                readonly property string selectedId: editing
                    ? (control.parameterContext?.editingParameterId ?? "")
                    : (control.parameterContext?.referenceParameterId ?? "")
                readonly property string selectedName: editing
                    ? (control.parameterContext?.editingParameterDisplayName ?? qsTr("None"))
                    : (control.parameterContext?.referenceParameterDisplayName ?? qsTr("None"))
                readonly property bool selectedWarning: selectedId.length > 0 && (editing
                    ? !(control.editingBinding?.registered ?? false)
                    : !(control.referenceBinding?.registered ?? false))

                implicitWidth: 132
                implicitHeight: 22
                model: control.parameterContext?.parameterModel ?? null
                textRole: "displayName"
                valueRole: "parameterId"
                displayText: currentIndex >= 0 ? currentText : selectedName

                Component.onCompleted: currentValue = Qt.binding(() => selectedId)
                onActivated: index => {
                    const value = valueAt(index)
                    if (editing)
                        control.parameterContext.editingParameterId = value
                    else
                        control.parameterContext.referenceParameterId = value
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

                        IconImage {
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

                        IconImage {
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
                onClicked: control.parameterContext?.swapParameters()
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
                checked: control.transformEditing
                enabled: control.editingBinding?.available ?? false
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
                    enabled: control.editingBinding?.available ?? false
                    onClicked: control.setTool(modelData.tool)
                }
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

        ScopicFlowInternal.ClipMappedProxyTimeViewModel {
            id: proxyTimeViewModel
            timeViewModel: control.contextObject?.timeViewModel ?? null
            clipViewModel: d.projectViewModelContext?.getClipViewItemFromDocumentItem(
                control.contextObject?.editingClip ?? null) ?? null
        }

        ParameterInfoProvider {
            id: parameterInfoProvider
            registry: CoreInterface.singerRegistry
            architectureId:
                control.parameterContext?.singingClip?.sources?.category ?? ""
            parameterId: control.editingBinding?.parameterId ?? ""
            transform: control.transformEditing
        }

        Binding {
            id: anchorCursorBinding
            target: control.contextObject?.timeLayoutViewModel ?? null
            property: "cursorPosition"
            value: (control.anchorBeingEdited?.position ?? 0)
                + (proxyTimeViewModel.clipViewModel?.position ?? 0)
                - (proxyTimeViewModel.clipViewModel?.clipStart ?? 0)
            when: false
        }

        Binding {
            id: freeEditCursorBinding
            target: control.contextObject?.timeLayoutViewModel ?? null
            property: "cursorPosition"
            value: control.freeEditPosition
                + (proxyTimeViewModel.clipViewModel?.position ?? 0)
                - (proxyTimeViewModel.clipViewModel?.clipStart ?? 0)
            when: false
        }

        ScopicFlowInternal.ParameterEditorContent {
            id: referenceLayer
            anchors.fill: parent
            z: 0
            visible: control.referenceBinding?.available ?? false
            freeParameterViewModel: control.referenceBinding?.freeEdited ?? null
            anchorParameterViewModel: control.referenceBinding?.anchorEdited ?? null
            originalParameterViewModel: control.referenceBinding?.original ?? null
            freeTransformParameterViewModel: control.referenceBinding?.freeTransform ?? null
            anchorTransformParameterViewModel: control.referenceBinding?.anchorTransform ?? null
            timeViewModel: proxyTimeViewModel
            timeLayoutViewModel: control.contextObject?.timeLayoutViewModel ?? null
            fillMode: control.referenceBinding?.interactionController?.fillMode
                ?? ScopicFlowInternal.ParameterEditorContent.NoFill
            fillBaseline: control.referenceBinding?.interactionController?.fillBaseline ?? 0.0
            referenceVisible: false
            defaultValueEnabled: control.referenceBinding?.interactionController?.defaultValueEnabled ?? false
            defaultValue: control.referenceBinding?.interactionController?.defaultValue ?? 0.0
            originalAndDefaultCurveDisplayMode: ScopicFlowInternal.ParameterEditorContent.CurveSolid
            editLayer: ScopicFlowInternal.ParameterEditorContent.FinalLayer
            curveColor: control.referenceCurveColor
            dimmedCurveColor: control.referenceCurveColor
            fillColor: control.referenceFillColor
            dimmedFillColor: control.referenceFillColor
            accentColor: control.referenceCurveColor
            referenceColor: control.referenceCurveColor
            selectedAnchorColor: control.referenceCurveColor
        }

        ScopicFlowInternal.ParameterEditorContent {
            id: finalParameterLayer
            anchors.fill: parent
            z: 1
            visible: control.transformEditing && (control.editingBinding?.available ?? false)
            freeParameterViewModel: control.editingBinding?.freeEdited ?? null
            anchorParameterViewModel: control.editingBinding?.anchorEdited ?? null
            originalParameterViewModel: control.editingBinding?.original ?? null
            freeTransformParameterViewModel: control.editingBinding?.freeTransform ?? null
            anchorTransformParameterViewModel: control.editingBinding?.anchorTransform ?? null
            timeViewModel: proxyTimeViewModel
            timeLayoutViewModel: control.contextObject?.timeLayoutViewModel ?? null
            fillMode: control.editingBinding?.interactionController?.fillMode
                ?? ScopicFlowInternal.ParameterEditorContent.NoFill
            fillBaseline: control.editingBinding?.interactionController?.fillBaseline ?? 0.0
            referenceVisible: false
            defaultValueEnabled: control.editingBinding?.interactionController?.defaultValueEnabled ?? false
            defaultValue: control.editingBinding?.interactionController?.defaultValue ?? 0.0
            originalAndDefaultCurveDisplayMode: ScopicFlowInternal.ParameterEditorContent.CurveSolid
            editLayer: ScopicFlowInternal.ParameterEditorContent.FinalLayer
            curveColor: control.finalCurveColor
            dimmedCurveColor: control.finalCurveColor
            fillColor: control.finalFillColor
            dimmedFillColor: control.finalFillColor
            accentColor: control.finalCurveColor
            referenceColor: control.finalCurveColor
            selectedAnchorColor: control.finalCurveColor
        }

        ScopicFlowInternal.ParameterEditorContent {
            id: transformDisplayLayer
            anchors.fill: parent
            z: 1
            visible: !control.transformEditing && (control.editingBinding?.available ?? false)
            freeParameterViewModel: control.editingBinding?.freeTransform ?? null
            anchorParameterViewModel: control.editingBinding?.anchorTransform ?? null
            timeViewModel: proxyTimeViewModel
            timeLayoutViewModel: control.contextObject?.timeLayoutViewModel ?? null
            fillMode: ScopicFlowInternal.ParameterEditorContent.NoFill
            referenceVisible: false
            defaultValueEnabled: true
            defaultValue: 0.5
            originalAndDefaultCurveDisplayMode: ScopicFlowInternal.ParameterEditorContent.CurveSolid
            editLayer: ScopicFlowInternal.ParameterEditorContent.FinalLayer
            curveColor: control.transformDisplayCurveColor
            dimmedCurveColor: control.transformDisplayCurveColor
            accentColor: control.transformDisplayCurveColor
            referenceColor: control.transformDisplayCurveColor
            selectedAnchorColor: control.transformDisplayCurveColor
        }

        ScopicFlowInternal.ParameterEditorContent {
            id: activeParameterFillLayer
            anchors.fill: parent
            z: 1
            visible: !control.transformEditing && (control.editingBinding?.available ?? false)
            freeParameterViewModel: control.editingBinding?.freeEdited ?? null
            anchorParameterViewModel: control.editingBinding?.anchorEdited ?? null
            originalParameterViewModel: control.editingBinding?.original ?? null
            freeTransformParameterViewModel: control.editingBinding?.freeTransform ?? null
            anchorTransformParameterViewModel: control.editingBinding?.anchorTransform ?? null
            timeViewModel: proxyTimeViewModel
            timeLayoutViewModel: control.contextObject?.timeLayoutViewModel ?? null
            fillMode: control.editingBinding?.interactionController?.fillMode
                ?? ScopicFlowInternal.ParameterEditorContent.NoFill
            fillBaseline: control.editingBinding?.interactionController?.fillBaseline ?? 0.0
            referenceVisible: false
            defaultValueEnabled: control.editingBinding?.interactionController?.defaultValueEnabled ?? false
            defaultValue: control.editingBinding?.interactionController?.defaultValue ?? 0.0
            originalAndDefaultCurveDisplayMode: ScopicFlowInternal.ParameterEditorContent.CurveHidden
            editLayer: ScopicFlowInternal.ParameterEditorContent.FinalLayer
            curveColor: control.transparentCurveColor
            dimmedCurveColor: control.transparentCurveColor
            fillColor: control.activeFillSupplementColor
            dimmedFillColor: control.activeFillSupplementColor
            accentColor: control.transparentCurveColor
            referenceColor: control.transparentCurveColor
            selectedAnchorColor: control.transparentCurveColor
        }

        ParameterEditor {
            id: editingEditor
            anchors.fill: parent
            z: 2
            visible: !control.transformEditing && (control.editingBinding?.available ?? false)
            freeParameterViewModel: control.editingBinding?.freeEdited ?? null
            anchorParameterViewModel: control.editingBinding?.anchorEdited ?? null
            originalParameterViewModel: control.editingBinding?.original ?? null
            freeTransformParameterViewModel: control.editingBinding?.freeTransform ?? null
            anchorTransformParameterViewModel: control.editingBinding?.anchorTransform ?? null
            freeParameterSelectionViewModel: control.editingBinding?.freeSelection ?? null
            anchorSelectionController: control.editingBinding?.anchorSelectionController ?? null
            interactionController: control.editingBinding?.interactionController ?? null
            timeViewModel: proxyTimeViewModel
            timeLayoutViewModel: control.contextObject?.timeLayoutViewModel ?? null
            Theme.accentColor: control.activeCurveColor
            Theme.foregroundSecondaryColor: control.dimmedEditingColor
            SFPalette.scaleSecondaryColor: control.defaultReferenceColor
        }

        ParameterEditor {
            id: transformEditor
            anchors.fill: parent
            z: 2
            visible: control.transformEditing && (control.editingBinding?.available ?? false)
            freeParameterViewModel: control.editingBinding?.freeTransform ?? null
            anchorParameterViewModel: control.editingBinding?.anchorTransform ?? null
            freeParameterSelectionViewModel: control.editingBinding?.transformFreeSelection ?? null
            anchorSelectionController: control.editingBinding?.transformAnchorSelectionController ?? null
            interactionController: control.editingBinding?.transformInteractionController ?? null
            timeViewModel: proxyTimeViewModel
            timeLayoutViewModel: control.contextObject?.timeLayoutViewModel ?? null
            Theme.accentColor: control.activeCurveColor
            Theme.foregroundPrimaryColor: control.transformCurveColor
            Theme.foregroundSecondaryColor: control.transformCurveColor
            SFPalette.scaleSecondaryColor: control.defaultReferenceColor
        }

        IconLabel {
            anchors.centerIn: parent
            z: 4
            visible: (control.parameterContext?.editingParameterId.length ?? 0) > 0
                && !(control.editingBinding?.registered ?? false)
            text: qsTr("The selected parameter is unavailable.")
            icon.source: "image://fluent-system-icons/warning"
            icon.width: 16
            icon.height: 16
            icon.color: Theme.foregroundPrimaryColor
            color: Theme.foregroundPrimaryColor
        }

        Item {
            id: scale
            width: 64
            height: parent.height
            z: 3
            visible: control.editingBinding?.available ?? false
            readonly property var activeParameterInfo: parameterInfoProvider.info
            readonly property bool valueIndicatorEditing:
                anchorCursorBinding.when || freeEditCursorBinding.when
            readonly property double valueIndicatorSource: anchorCursorBinding.when
                ? (control.anchorBeingEdited?.value ?? 0.0)
                : freeEditCursorBinding.when
                    ? control.freeEditValue
                    : control.hoveredParameterValue
            readonly property double valueIndicatorPosition: {
                if (!Number.isFinite(valueIndicatorSource))
                    return 0.0
                return Math.max(0.0, Math.min(1.0, valueIndicatorSource))
            }
            readonly property var valueIndicatorRawValue: {
                const info = parameterInfoProvider.info
                if (!parameterInfoProvider.exists
                        || info === undefined || info === null
                        || !Number.isFinite(valueIndicatorSource)) {
                    return undefined
                }
                return info.invokeDenormalize(valueIndicatorPosition)
            }
            readonly property string valueIndicatorDisplayString: {
                const info = parameterInfoProvider.info
                if (!parameterInfoProvider.exists
                        || info === undefined || info === null
                        || valueIndicatorRawValue === undefined
                        || valueIndicatorRawValue === null) {
                    return ""
                }
                return parameterInfoProvider.displayString(valueIndicatorRawValue)
            }

            Loader {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                width: 8
                active: control.editingBinding?.available ?? false
                sourceComponent: ParameterDivisionItem {
                    parameterInfo: scale.activeParameterInfo
                    color: Theme.foregroundPrimaryColor
                    lineLength: 8
                }
            }
            Label {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 3
                text: scale.activeParameterInfo?.invokeToDisplayString(
                    scale.activeParameterInfo.topValue) ?? ""
            }
            Label {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 3
                text: scale.activeParameterInfo?.invokeToDisplayString(
                    scale.activeParameterInfo.bottomValue) ?? ""
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
                visible: parameterInfoProvider.exists
                    && (scale.valueIndicatorEditing || control.parameterHovered)
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

        Component.onCompleted: setTool(currentTool)
        Connections {
            target: control.editingBinding
            function onTargetChanged() {
                control.parameterHovered = false
                control.setTool(control.currentTool)
            }
        }
        Connections {
            target: control.editingBinding?.interactionController ?? null
            function onFreeEditingStarted(editor, operation, position, value) {
                control.beginFreeEdit(editor, position, value)
            }
            function onFreeEditingUpdated(editor, operation, position, value) {
                control.updateFreeEdit(editor, position, value)
            }
            function onFreeEditingCommitted(editor) {
                control.endFreeEdit(editor)
            }
            function onFreeEditingAborted(editor) {
                control.endFreeEdit(editor)
            }
            function onFreeRangeSelectingStarted(editor) {
                if (editor === editingEditor)
                    control.beginCursorPositionHidingSelection(editor)
            }
            function onFreeRangeSelectingCommitted(editor) {
                if (editor === editingEditor)
                    control.endCursorPositionHidingSelection(editor)
            }
            function onFreeRangeSelectingAborted(editor) {
                if (editor === editingEditor)
                    control.endCursorPositionHidingSelection(editor)
            }
            function onAnchorRubberBandDraggingStarted(editor) {
                if (editor === editingEditor)
                    control.beginCursorPositionHidingSelection(editor)
            }
            function onAnchorRubberBandDraggingCommitted(editor) {
                if (editor === editingEditor)
                    control.endCursorPositionHidingSelection(editor)
            }
            function onAnchorRubberBandDraggingAborted(editor) {
                if (editor === editingEditor)
                    control.endCursorPositionHidingSelection(editor)
            }
            function onAnchorMovingStarted(editor, item) {
                control.beginAnchorMove(editor, item)
            }
            function onAnchorMovingCommitted(editor) {
                control.endAnchorMove(editor)
            }
            function onAnchorMovingAborted(editor) {
                control.endAnchorMove(editor)
            }
            function onHoverEntered(editor, position, value) {
                control.updateParameterHover(editor, value)
            }
            function onHoverMoved(editor, position, value) {
                control.updateParameterHover(editor, value)
            }
            function onHoverExited(editor) {
                control.endParameterHover(editor)
            }
        }
        Connections {
            target: control.editingBinding?.transformInteractionController ?? null
            function onFreeEditingStarted(editor, operation, position, value) {
                control.beginFreeEdit(editor, position, value)
            }
            function onFreeEditingUpdated(editor, operation, position, value) {
                control.updateFreeEdit(editor, position, value)
            }
            function onFreeEditingCommitted(editor) {
                control.endFreeEdit(editor)
            }
            function onFreeEditingAborted(editor) {
                control.endFreeEdit(editor)
            }
            function onFreeRangeSelectingStarted(editor) {
                if (editor === transformEditor)
                    control.beginCursorPositionHidingSelection(editor)
            }
            function onFreeRangeSelectingCommitted(editor) {
                if (editor === transformEditor)
                    control.endCursorPositionHidingSelection(editor)
            }
            function onFreeRangeSelectingAborted(editor) {
                if (editor === transformEditor)
                    control.endCursorPositionHidingSelection(editor)
            }
            function onAnchorRubberBandDraggingStarted(editor) {
                if (editor === transformEditor)
                    control.beginCursorPositionHidingSelection(editor)
            }
            function onAnchorRubberBandDraggingCommitted(editor) {
                if (editor === transformEditor)
                    control.endCursorPositionHidingSelection(editor)
            }
            function onAnchorRubberBandDraggingAborted(editor) {
                if (editor === transformEditor)
                    control.endCursorPositionHidingSelection(editor)
            }
            function onAnchorMovingStarted(editor, item) {
                control.beginAnchorMove(editor, item)
            }
            function onAnchorMovingCommitted(editor) {
                control.endAnchorMove(editor)
            }
            function onAnchorMovingAborted(editor) {
                control.endAnchorMove(editor)
            }
            function onHoverEntered(editor, position, value) {
                control.updateParameterHover(editor, value)
            }
            function onHoverMoved(editor, position, value) {
                control.updateParameterHover(editor, value)
            }
            function onHoverExited(editor) {
                control.endParameterHover(editor)
            }
        }
        Connections {
            target: control.parameterContext
            function onTransformEditingChanged() {
                control.parameterHovered = false
                control.setTool(control.currentTool)
            }
        }
    }
}
