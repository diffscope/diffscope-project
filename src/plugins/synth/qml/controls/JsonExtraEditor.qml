// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import "JsonState.js" as JsonState

Item {
    id: control

    signal stateEdited()

    onVisibleChanged: {
        if (!visible && editDialog?.visible)
            editDialog.reject()
    }

    QtObject {
        id: d

        property var committedValue: null
        property bool loaded: false
        property string errorMessage: ""
    }

    function loadState(value) {
        const copied = JsonState.copy(value)
        if (d.loaded && JSON.stringify(copied) === JSON.stringify(d.committedValue))
            return
        if (editDialog.visible)
            editDialog.reject()
        d.committedValue = copied
        d.loaded = true
    }

    function saveState() {
        return JsonState.copy(d.committedValue)
    }

    function submitDraft() {
        let value
        try {
            value = JsonState.copy(JSON.parse(draftEditor.text))
        } catch (error) {
            d.errorMessage = qsTr("The settings are not valid. Check the text and try again.")
            draftEditor.forceActiveFocus()
            return
        }
        const changed = JSON.stringify(value) !== JSON.stringify(d.committedValue)
        d.committedValue = value
        editDialog.accept()
        if (changed)
            stateEdited()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TextArea {
                text: JSON.stringify(d.committedValue, null, 4)
                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.NoWrap
                Accessible.name: qsTr("Settings")
            }
        }

        Button {
            Layout.alignment: Qt.AlignTrailing
            icon.source: "image://fluent-system-icons/edit"
            text: qsTr("Edit JSON (advanced)...")
            enabled: d.loaded
            onClicked: editDialog.open()
        }
    }

    Dialog {
        id: editDialog

        parent: control.Overlay.overlay
        anchors.centerIn: parent
        width: parent ? Math.min(640, parent.width - 48) : 640
        height: parent ? Math.min(480, parent.height - 48) : 480
        modal: true
        closePolicy: Popup.CloseOnEscape
        title: qsTr("Edit JSON")

        onAboutToShow: {
            draftEditor.text = JSON.stringify(d.committedValue, null, 4)
            d.errorMessage = ""
        }
        onOpened: draftEditor.forceActiveFocus()

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                TextArea {
                    id: draftEditor
                    selectByMouse: true
                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.NoWrap
                    Accessible.name: qsTr("Settings")
                    onTextChanged: d.errorMessage = ""
                }
            }

            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: d.errorMessage
                color: Theme.errorColor
                wrapMode: Text.Wrap
            }
        }

        footer: DialogButtonBox {
            Button {
                text: qsTr("Apply Changes")
                ThemedItem.controlType: SVS.CT_Accent
                DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
                onClicked: control.submitDraft()
            }
            Button {
                text: qsTr("Cancel")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
            onRejected: editDialog.reject()
        }
    }
}
