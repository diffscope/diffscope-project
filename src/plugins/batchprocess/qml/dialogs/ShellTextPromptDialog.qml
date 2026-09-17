// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Dialog {
    id: dialog

    property string labelText
    property string inputText
    property string placeholderText
    property bool multiline
    property bool allowEmpty: true
    property bool attempted

    readonly property bool inputValid: allowEmpty || inputText.trim().length !== 0

    width: 480
    modal: true
    closePolicy: Popup.CloseOnEscape

    function submit() {
        attempted = true
        if (!inputValid) {
            inputLoader.item.forceActiveFocus()
            return
        }
        accept()
    }

    onAboutToShow: inputLoader.item.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: dialog.labelText
            wrapMode: Text.Wrap
        }

        Loader {
            id: inputLoader
            Layout.fillWidth: true
            sourceComponent: dialog.multiline ? multilineEditor : singleLineEditor
        }

        Label {
            Layout.fillWidth: true
            visible: dialog.attempted && !dialog.inputValid
            color: Theme.errorColor
            text: qsTr("A value is required.")
            wrapMode: Text.Wrap
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTranslate("QPlatformTheme", "OK")
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: dialog.submit()
        }
        Button {
            text: qsTranslate("QPlatformTheme", "Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: dialog.reject()
        }
    }

    Component {
        id: singleLineEditor

        TextField {
            text: dialog.inputText
            placeholderText: dialog.placeholderText
            ThemedItem.controlType: dialog.attempted && !dialog.inputValid ? SVS.CT_Error : SVS.CT_Normal
            onTextEdited: dialog.inputText = text
            Keys.onReturnPressed: dialog.submit()
        }
    }

    Component {
        id: multilineEditor

        ScrollView {
            implicitHeight: 160

            TextArea {
                text: dialog.inputText
                placeholderText: dialog.placeholderText
                wrapMode: TextEdit.Wrap
                ThemedItem.controlType: dialog.attempted && !dialog.inputValid ? SVS.CT_Error : SVS.CT_Normal
                onTextChanged: dialog.inputText = text
            }
        }
    }
}
