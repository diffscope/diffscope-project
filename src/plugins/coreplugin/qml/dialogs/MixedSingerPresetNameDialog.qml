// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

Dialog {
    id: dialog

    property string presetId
    property string initialName
    property SourcesPickerModel sourcesModel: null
    property var modelIndex: null
    property bool saveFailed: false

    title: presetId === "" ? qsTr("Save Mixed Singer Preset") : qsTr("Rename Preset")
    width: Math.min(360, parent ? parent.width : 360)
    modal: true
    closePolicy: Popup.CloseOnEscape

    function savePreset() {
        const saved = presetId === ""
                      ? CoreInterface.mixedSingerPresets.savePreset(nameField.text, sourcesModel, modelIndex)
                      : CoreInterface.mixedSingerPresets.renamePreset(presetId, nameField.text)
        if (saved)
            accept()
        else
            saveFailed = true
    }

    onAboutToShow: {
        saveFailed = false
        nameField.text = initialName
        nameField.selectAll()
        nameField.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: qsTr("Preset name")
        }

        TextField {
            id: nameField
            Layout.fillWidth: true
            Accessible.name: qsTr("Preset name")
            onAccepted: {
                if (text.trim().length > 0)
                    dialog.savePreset()
            }
        }

        Annotation {
            Layout.fillWidth: true
            visible: dialog.saveFailed
            ThemedItem.controlType: SVS.CT_Warning
            label: qsTr("The preset could not be saved. The singer may no longer be available.")
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Save")
            enabled: nameField.text.trim().length > 0
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: dialog.savePreset()
        }

        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }

        onRejected: dialog.reject()
    }
}
