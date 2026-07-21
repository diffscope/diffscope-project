pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views
import dev.sjimo.ScopicFlow.Internal as ScopicFlowInternal

import DiffScope.Core
import DiffScope.DspxModel as DspxModel

QtObject {
    id: d

    required property QtObject addOn

    readonly property ProjectViewModelContext projectViewModelContext:
        addOn?.windowHandle.ProjectViewModelContext.context ?? null

    readonly property Component dynamicMixingTrackComponent: Item {
        id: control

        required property PianoRollPanelInterface contextObject

        property int currentTool: DynamicMixingEditorInteractionController.Pointer
        readonly property DynamicMixingEditorContext dynamicMixingContext:
            d.projectViewModelContext?.dynamicMixingEditorContext ?? null
        readonly property var singers:
            dynamicMixingContext?.singingClip?.sources?.singers?.items ?? []
        readonly property var colors: {
            const source = CoreInterface.trackColorSchema.colors ?? []
            const count = source.length
            if (count === 0)
                return []
            const result = []
            for (let index = 0; index < count; ++index) {
                const reorderedIndex = index % 2
                    ? Math.floor((count + index) / 2)
                    : Math.floor(index / 2)
                result.push(source[reorderedIndex])
            }
            return result
        }

        function colorAt(index): color {
            return colors.length > 0 ? colors[index % colors.length]
                                     : Theme.accentColor
        }

        function resetController() {
            const controller = dynamicMixingContext?.interactionController
            if (!controller)
                return
            controller.primaryItemInteraction = DynamicMixingEditorInteractionController.None
            controller.secondaryItemInteraction = DynamicMixingEditorInteractionController.None
            controller.primarySceneInteraction = DynamicMixingEditorInteractionController.None
            controller.secondarySceneInteraction = DynamicMixingEditorInteractionController.None
            controller.primarySelectInteraction = DynamicMixingEditorInteractionController.None
            controller.secondarySelectInteraction = DynamicMixingEditorInteractionController.None
        }

        function setTool(tool: int) {
            currentTool = tool
            resetController()
            const controller = dynamicMixingContext?.interactionController
            if (!controller)
                return
            controller.primaryItemInteraction = tool
            controller.secondaryItemInteraction = tool
            controller.primarySceneInteraction = tool
            controller.secondarySceneInteraction = tool
            controller.primarySelectInteraction = tool
            controller.secondarySelectInteraction = tool
            controller.clickSelectable = true
        }

        readonly property Item toolBar: RowLayout {
            spacing: 4

            Repeater {
                model: [
                    { text: qsTr("Pointer"), icon: "cursor",
                      tool: DynamicMixingEditorInteractionController.Pointer },
                    { text: qsTr("Add anchor"), icon: "calligraphy_pen_add",
                      tool: DynamicMixingEditorInteractionController.AddAnchor },
                    { text: qsTr("Delete anchor"), icon: "calligraphy_pen_subtract",
                      tool: DynamicMixingEditorInteractionController.DeleteAnchor },
                    { text: qsTr("Rubber-band select"), icon: "rectangle_landscape_dash",
                      tool: DynamicMixingEditorInteractionController.RubberBandSelect }
                ]

                delegate: ToolButton {
                    required property var modelData

                    implicitWidth: 22
                    implicitHeight: 22
                    padding: 3
                    text: modelData.text
                    icon.source: "image://fluent-system-icons/" + modelData.icon
                    display: AbstractButton.IconOnly
                    checkable: true
                    checked: control.currentTool === modelData.tool
                    enabled: control.dynamicMixingContext?.available ?? false
                    ButtonGroup.group: toolGroup
                    ToolTip.visible: hovered
                    ToolTip.text: text
                    onClicked: control.setTool(modelData.tool)
                }
            }

            ButtonGroup {
                id: toolGroup
                exclusive: true
            }

            Item {
                Layout.fillWidth: true
            }

            Repeater {
                model: control.singers

                delegate: RowLayout {
                    id: legendEntry

                    required property int index
                    required property var modelData

                    spacing: 3

                    readonly property bool mixed:
                        modelData?.type === DspxModel.Singer.Mixed
                    readonly property string singerId:
                        mixed ? "" : (modelData?.id ?? "")
                    readonly property string displayName: mixed
                        ? qsTr("Mixed singer")
                        : singerInfoProvider.info.name || singerId
                          || qsTr("Unnamed singer")

                    SingerInfoProvider {
                        id: singerInfoProvider
                        registry: CoreInterface.singerRegistry
                        architectureId:
                            control.dynamicMixingContext?.singingClip?.sources?.category ?? ""
                        singerId: legendEntry.singerId
                    }

                    Rectangle {
                        Layout.preferredWidth: 9
                        Layout.preferredHeight: 9
                        radius: width / 2
                        color: control.colorAt(legendEntry.index)
                    }

                    Label {
                        Layout.maximumWidth: 112
                        elide: Text.ElideRight
                        text: legendEntry.displayName
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                    }
                }
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

        DynamicMixingEditor {
            anchors.fill: parent
            visible: control.dynamicMixingContext?.available ?? false
            dynamicMixingViewModel:
                control.dynamicMixingContext?.dynamicMixingViewModel ?? null
            selectionController:
                control.dynamicMixingContext?.selectionController ?? null
            interactionController:
                control.dynamicMixingContext?.interactionController ?? null
            timeViewModel: proxyTimeViewModel
            timeLayoutViewModel: control.contextObject?.timeLayoutViewModel ?? null
            colors: control.colors.map(c => Qt.rgba(c.r, c.g, c.b, c.a * 0.25))
        }

        IconLabel {
            anchors.centerIn: parent
            visible: !(control.dynamicMixingContext?.available ?? false)
            text: qsTr("Voice blending is unavailable for this clip.")
            icon.source: "image://fluent-system-icons/warning"
            icon.width: 16
            icon.height: 16
            icon.color: Theme.foregroundSecondaryColor
            color: Theme.foregroundSecondaryColor
        }

        Component.onCompleted: setTool(currentTool)
        onDynamicMixingContextChanged: setTool(currentTool)
        Connections {
            target: control.dynamicMixingContext
            function onSingingClipChanged() { control.setTool(control.currentTool) }
        }
    }
}
