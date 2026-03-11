import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

Dialog {
    id: dialog

    property string token
    property int start
    property string language
    property bool onset

    title: qsTr("Insert Phoneme")

    onAboutToShow: tokenTextField.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2

        Label {
            id: tokenLabel
            text: qsTr("Token")
        }
        TextField {
            id: tokenTextField
            Accessible.labelledBy: tokenLabel
            Accessible.name: tokenLabel.text
            Layout.fillWidth: true
            text: dialog.token
            onTextEdited: dialog.token = text
        }

        Label {
            id: startLabel
            text: qsTr("Start (ms)")
        }
        SpinBox {
            id: startSpinBox
            Accessible.labelledBy: startLabel
            Accessible.name: startLabel.text
            Layout.fillWidth: true
            value: dialog.start
            from: -2147483648
            to: 2147483647
            onValueModified: dialog.start = value
        }

        Label {
            id: languageLabel
            text: qsTr("Language")
        }
        TextField {
            id: languageTextField
            Accessible.labelledBy: languageLabel
            Accessible.name: languageLabel.text
            Layout.fillWidth: true
            text: dialog.language
            onTextEdited: dialog.language = text
        }

        CheckBox {
            id: onsetCheckBox
            Layout.columnSpan: 2
            Layout.fillWidth: true
            text: qsTr("Onset")
            checked: dialog.onset
            onClicked: dialog.onset = checked
        }
    }

    standardButtons: DialogButtonBox.Ok
}
