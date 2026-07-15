import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

Item {
    id: control

    required property SourcesPickerModel sourcesModel
    required property var modelIndex

    readonly property bool modelIndexValid: {
        sourcesModel.revision
        return Boolean(modelIndex && sourcesModel.indexAlive(modelIndex))
    }
    readonly property bool mixedValid: {
        sourcesModel.revision
        return modelIndexValid && sourcesModel.singerValid(modelIndex)
    }
    readonly property string mixedGroup: {
        sourcesModel.revision
        return modelIndexValid ? sourcesModel.effectiveMixGroup(modelIndex) : ""
    }
    readonly property string currentName: {
        sourcesModel.revision
        return modelIndexValid ? sourcesModel.displayName(modelIndex) : ""
    }

    function percentageAt(row) {
        sourcesModel.revision
        if (!modelIndexValid)
            return 0
        const values = sourcesModel.ratios(modelIndex)
        let result = []
        let order = []
        let used = 0
        for (let i = 0; i < values.length; ++i) {
            const exact = Number(values[i]) * 100
            const base = Math.floor(exact)
            result.push(base)
            used += base
            order.push({ index: i, fraction: exact - base })
        }
        order.sort((a, b) => b.fraction - a.fraction || a.index - b.index)
        for (let i = 0; i < 100 - used && i < order.length; ++i)
            result[order[i].index] += 1
        return Number(result[row] ?? 0)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                id: nameLabel
                Layout.maximumWidth: Math.max(80, control.width / 2)
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                text: control.currentName
            }

            ToolButton {
                flat: true
                display: AbstractButton.IconOnly
                text: qsTr("Rename mixed singer")
                icon.source: "image://fluent-system-icons/edit"
                ToolTip.visible: hovered
                ToolTip.text: text
                onClicked: nameEditorPopup.open()
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                flat: true
                display: AbstractButton.IconOnly
                text: qsTr("Load preset")
                icon.source: "image://fluent-system-icons/folder_open"
                ToolTip.visible: hovered
                ToolTip.text: text
            }

            ToolButton {
                flat: true
                display: AbstractButton.IconOnly
                text: qsTr("Save preset")
                icon.source: "image://fluent-system-icons/save"
                ToolTip.visible: hovered
                ToolTip.text: text
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 0

            ListView {
                id: singerListView
                anchors.fill: parent
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: DelegateModel {
                    model: control.sourcesModel
                    rootIndex: control.modelIndex

                    delegate: Rectangle {
                        id: singerRow

                        required property int index
                        required property int singerType
                        required property string singerId
                        required property var singerTree
                        required property string displayName
                        required property real ratio
                        required property bool singerValid
                        required property string warningText
                        required property var modelIndex

                        property int dragIndex: index

                        width: singerListView.width
                        height: 32
                        color: rowMouseArea.containsMouse ? Theme.controlHoveredColorChange.apply(Theme.backgroundSecondaryColor)
                                                           : Theme.backgroundSecondaryColor
                        Drag.active: rowDragHandler.active
                        Drag.source: singerRow
                        Drag.hotSpot.x: width / 2
                        Drag.hotSpot.y: height / 2
                        z: rowDragHandler.active ? 2 : 0

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 4
                            anchors.rightMargin: 2
                            spacing: 6

                            SingerAvatar {
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                                architectureId: control.sourcesModel.architectureId
                                singerTree: singerRow.singerTree
                            }

                            Label {
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                text: singerRow.displayName
                            }

                            IconLabel {
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                                visible: !singerRow.singerValid
                                icon.source: "image://fluent-system-icons/warning"
                                icon.color: Theme.warningColor
                                icon.width: 18
                                icon.height: 18
                                ToolTip.visible: warningHover.hovered
                                ToolTip.text: singerRow.warningText

                                HoverHandler {
                                    id: warningHover
                                }
                            }

                            Label {
                                Layout.preferredWidth: 48
                                horizontalAlignment: Text.AlignRight
                                text: qsTr("%1%").arg(control.percentageAt(singerRow.index))
                            }

                            ToolButton {
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 28
                                flat: true
                                display: AbstractButton.IconOnly
                                text: qsTr("Configure singer")
                                icon.source: "image://fluent-system-icons/settings"
                                enabled: singerRow.singerType === SourcesPickerModel.MixedSinger
                                         || singerRow.singerValid
                                ToolTip.visible: hovered
                                ToolTip.text: enabled ? text : singerRow.warningText
                                onClicked: {
                                    configurationPopup.targetIndex = singerRow.modelIndex
                                    configurationPopup.open()
                                }
                            }

                            ToolButton {
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 28
                                flat: true
                                display: AbstractButton.IconOnly
                                text: qsTr("Replace singer")
                                icon.source: "image://fluent-system-icons/arrow_swap"
                                enabled: control.mixedValid && control.mixedGroup !== ""
                                ToolTip.visible: hovered
                                ToolTip.text: enabled ? text : qsTr("An invalid mixed singer cannot replace entries.")
                                onClicked: {
                                    replacementPopup.targetIndex = singerRow.modelIndex
                                    replacementPopup.open()
                                }
                            }

                            ToolButton {
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 28
                                flat: true
                                display: AbstractButton.IconOnly
                                text: qsTr("Remove singer")
                                icon.source: "image://fluent-system-icons/delete"
                                enabled: singerListView.count > 1
                                ToolTip.visible: hovered
                                ToolTip.text: enabled ? text : qsTr("A mixed singer must retain at least one child singer.")
                                onClicked: control.sourcesModel.removeSinger(singerRow.modelIndex)
                            }
                        }

                        MouseArea {
                            id: rowMouseArea
                            anchors.fill: parent
                            anchors.rightMargin: 90
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }

                        DragHandler {
                            id: rowDragHandler
                            target: null
                        }

                        DropArea {
                            anchors.fill: parent
                            onEntered: drag => {
                                if (drag.source && drag.source !== singerRow)
                                    control.sourcesModel.moveSinger(drag.source.modelIndex, singerRow.index)
                            }
                        }
                    }
                }

                footer: Item {
                    width: singerListView.width
                    height: 36

                    Button {
                        anchors.centerIn: parent
                        flat: true
                        text: qsTr("Add singer")
                        icon.source: "image://fluent-system-icons/add"
                        enabled: control.mixedValid && control.mixedGroup !== ""
                        ToolTip.visible: hovered && !enabled
                        ToolTip.text: qsTr("An invalid mixed singer or empty mix group cannot accept another singer.")
                        onClicked: appendSingerPopup.open()
                    }
                }

                displaced: Transition {
                    NumberAnimation {
                        properties: "y"
                        duration: 120
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }

        SegmentedSingerRatioSlider {
            Layout.fillWidth: true
            sourcesModel: control.sourcesModel
            mixedSingerIndex: control.modelIndex
        }
    }

    Popup {
        id: nameEditorPopup

        property bool cancelled: false

        x: nameLabel.x
        y: nameLabel.y
        width: Math.max(220, nameLabel.width + 48)
        padding: 0
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        onAboutToShow: {
            if (!control.modelIndexValid)
                return
            cancelled = false
            nameEditor.text = control.sourcesModel.workspaceName(control.modelIndex)
                              || control.currentName
            nameEditor.selectAll()
            nameEditor.forceActiveFocus()
        }
        onAboutToHide: {
            if (!cancelled && control.modelIndexValid)
                control.sourcesModel.setMixedName(control.modelIndex, nameEditor.text)
        }

        TextField {
            id: nameEditor
            width: parent.width
            Accessible.name: qsTr("Mixed singer name")
            Keys.onReturnPressed: nameEditorPopup.close()
            Keys.onEnterPressed: nameEditorPopup.close()
            Keys.onEscapePressed: {
                nameEditorPopup.cancelled = true
                nameEditorPopup.close()
            }
        }
    }

    Popup {
        id: configurationPopup

        property var targetIndex: null
        readonly property bool targetIndexValid: {
            control.sourcesModel.revision
            return Boolean(targetIndex && control.sourcesModel.indexAlive(targetIndex))
        }
        readonly property int targetType: targetIndexValid
                                          ? control.sourcesModel.singerType(targetIndex)
                                          : SourcesPickerModel.InvalidSinger

        parent: control
        x: Math.max(0, (control.width - width) / 2)
        y: Math.max(0, (control.height - height) / 2)
        width: Math.min(560, control.width)
        height: Math.min(440, control.height)
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        onAboutToShow: {
            if (!targetIndexValid)
                return
            const componentUrl = targetType === SourcesPickerModel.MixedSinger
                                 ? Qt.resolvedUrl("MixedSingerEditor.qml")
                                 : Qt.resolvedUrl("SourcesSingerControlPanel.qml")
            configurationLoader.setSource(componentUrl, {
                sourcesModel: control.sourcesModel,
                modelIndex: targetIndex
            })
        }
        onClosed: configurationLoader.source = ""

        Loader {
            id: configurationLoader
            anchors.fill: parent
        }
    }

    Popup {
        id: appendSingerPopup

        parent: control
        x: Math.max(0, (control.width - width) / 2)
        y: Math.max(0, (control.height - height) / 2)
        width: Math.min(520, control.width)
        height: Math.min(420, control.height)
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        SingerPicker {
            anchors.fill: parent
            architectureId: control.sourcesModel.architectureId
            mixGroup: control.mixedGroup

            onSingerSelected: (architectureId, singerId) => {
                if (control.sourcesModel.tryAppendSingerToMixed(control.modelIndex, architectureId, singerId))
                    appendSingerPopup.close()
            }
        }
    }

    Popup {
        id: replacementPopup

        property var targetIndex: null
        readonly property bool targetIndexValid: {
            control.sourcesModel.revision
            return Boolean(targetIndex && control.sourcesModel.indexAlive(targetIndex))
        }
        readonly property string targetMixGroup: {
            control.sourcesModel.revision
            return targetIndexValid
                   ? control.sourcesModel.effectiveMixGroup(targetIndex)
                   : ""
        }

        parent: control
        x: Math.max(0, (control.width - width) / 2)
        y: Math.max(0, (control.height - height) / 2)
        width: Math.min(520, control.width)
        height: Math.min(420, control.height)
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onClosed: targetIndex = null

        SingerPicker {
            anchors.fill: parent
            enabled: replacementPopup.targetIndexValid
                     && control.mixedValid
                     && replacementPopup.targetMixGroup !== ""
            architectureId: control.sourcesModel.architectureId
            mixGroup: replacementPopup.targetMixGroup

            onSingerSelected: (architectureId, singerId) => {
                if (replacementPopup.targetIndexValid
                        && control.sourcesModel.tryReplaceSinger(replacementPopup.targetIndex, architectureId, singerId)) {
                    replacementPopup.close()
                }
            }
        }
    }
}
