import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

Dialog {
    id: dialog

    property string name
    property string author
    property int centShift: 0

    title: qsTr("Edit Metadata")

    onAboutToShow: nameField.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2
        columnSpacing: 12
        rowSpacing: 8

        Label {
            id: titleLabel
            text: qsTr("Title")
        }
        TextField {
            id: nameField
            Accessible.labelledBy: titleLabel
            Accessible.name: titleLabel.text
            Layout.fillWidth: true
            text: dialog.name
            onTextChanged: dialog.name = text
        }

        Label {
            id: authorLabel
            text: qsTr("Author")
        }
        TextField {
            id: authorField
            Accessible.labelledBy: authorLabel
            Accessible.name: authorLabel.text
            Layout.fillWidth: true
            text: dialog.author
            onTextChanged: dialog.author = text
        }

        Label {
            id: centShiftLabel
            text: qsTr("Cent shift")
        }
        SpinBox {
            id: centShiftSpinBox
            Accessible.labelledBy: centShiftLabel
            Accessible.name: centShiftLabel.text
            Layout.fillWidth: true
            value: dialog.centShift
            from: -50
            to: 50
            stepSize: 1
            onValueModified: dialog.centShift = value
        }
    }

    standardButtons: DialogButtonBox.Ok
}
