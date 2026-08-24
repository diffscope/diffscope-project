// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

Dialog {
    id: dialog

    required property QtObject addOn

    readonly property bool canUsePresets:
        (addOn?.hasEffectsContext ?? false) && (addOn?.availableEffects.length ?? 0) > 0
    readonly property int currentPresetIndex: presetList.currentIndex
    readonly property string currentPresetName:
        currentPresetIndex >= 0 ? (addOn?.presetNames ?? [])[currentPresetIndex] : ""

    title: qsTr("Effect Presets")
    width: 480
    height: 460
    modal: false

    footer: DialogButtonBox {
        Button {
            text: qsTr("Apply Preset")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            ThemedItem.controlType: SVS.CT_Accent
            enabled: dialog.canUsePresets
        }
        Button {
            text: qsTr("Close")
            DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
        }
    }

    onAccepted: {
        if (canUsePresets && currentPresetIndex >= 0)
            dialog.addOn.applyPreset(currentPresetName)
    }

    function openSaveAsDialog() {
        saveAsDialog.presetName = ""
        saveAsDialog.open()
    }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        spacing: 8

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                ListView {
                    id: presetList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    focus: true
                    model: dialog.addOn?.presetNames ?? []
                    currentIndex: -1

                    onCountChanged: {
                        if (count === 0)
                            currentIndex = -1
                        else if (currentIndex < 0)
                            currentIndex = 0
                        else if (currentIndex >= count)
                            currentIndex = count - 1
                    }

                    delegate: ItemDelegate {
                        id: presetDelegate
                        required property string modelData
                        required property int index

                        width: ListView.view.width
                        padding: 4
                        leftPadding: 8
                        rightPadding: 8
                        highlighted: ListView.isCurrentItem
                        ThemedItem.flat: true
                        ThemedItem.controlType: highlighted ? SVS.CT_Accent : SVS.CT_Normal
                        background: ButtonRectangle {
                            control: presetDelegate
                            checked: presetDelegate.highlighted
                            flat: true
                        }
                        contentItem: Label {
                            text: presetDelegate.modelData
                            elide: Text.ElideRight
                        }
                        onClicked: presetList.currentIndex = index
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: presetList.count === 0
                        text: qsTr("No presets")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: Theme.borderColor
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 4
                    spacing: 2

                    ToolButton {
                        text: qsTr("Save As Preset")
                        display: AbstractButton.IconOnly
                        icon.source: "image://fluent-system-icons/save"
                        enabled: dialog.canUsePresets
                        onClicked: dialog.openSaveAsDialog()
                    }
                    ToolButton {
                        text: qsTr("Delete Preset")
                        display: AbstractButton.IconOnly
                        icon.source: "image://fluent-system-icons/delete"
                        enabled: dialog.currentPresetIndex >= 0
                        onClicked: {
                            if (contentColumn.MessageBox.question(qsTr("Delete Preset"),
                                qsTr("Are you sure you want to delete preset \"%1\"?").arg(dialog.currentPresetName),
                                SVS.Yes | SVS.No, SVS.No) === SVS.Yes) {
                                dialog.addOn.deletePreset(dialog.currentPresetName)
                            }
                        }
                    }
                    Item { Layout.fillWidth: true }
                }
            }
        }
    }

    Dialog {
        id: saveAsDialog
        anchors.centerIn: parent
        width: 400
        title: qsTr("Save Preset As")
        property string presetName: ""
        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            Label {
                Layout.fillWidth: true
                text: qsTr("Preset name")
            }
            TextField {
                id: presetNameTextField
                Layout.fillWidth: true
                text: saveAsDialog.presetName
                onTextEdited: saveAsDialog.presetName = text
                Keys.onReturnPressed: saveAsOkButton.animateClick()
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("A preset with this name already exists. Overwrite it?")
                color: Theme.warningColor
                visible: dialog.addOn?.hasPreset(saveAsDialog.presetName) ?? false
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button {
                    id: saveAsOkButton
                    ThemedItem.controlType: SVS.CT_Accent
                    text: qsTr("OK")
                    enabled: saveAsDialog.presetName.trim().length > 0
                    onClicked: {
                        const name = saveAsDialog.presetName.trim()
                        const overwrite = dialog.addOn?.hasPreset(name) ?? false
                        if (overwrite) {
                            const choice = contentColumn.MessageBox.question(qsTr("Overwrite Preset"),
                                qsTr("A preset with the name \"%1\" already exists. Overwrite it?").arg(name),
                                SVS.Yes | SVS.No, SVS.No)
                            if (choice !== SVS.Yes)
                                return
                        }
                        if (dialog.addOn.savePreset(name)) {
                            Qt.callLater(() => {
                                const index = dialog.addOn.presetIndex(name)
                                if (index >= 0) {
                                    presetList.currentIndex = index
                                    presetList.positionViewAtIndex(index, ListView.Contain)
                                }
                            })
                        }
                        saveAsDialog.close()
                    }
                }
            }
        }
        onAboutToShow: () => {
            presetName = ""
            presetNameTextField.forceActiveFocus()
        }
    }
}
