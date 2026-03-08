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

    property MusicTimeline timeline
    property int labelPos
    property string labelText

    title: qsTr("Insert Label")

    onAboutToShow: labelTextField.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2
        Label {
            id: labelTextLabel
            text: qsTr("Text")
        }
        TextField {
            id: labelTextField
            Accessible.labelledBy: labelTextLabel
            Accessible.name: labelTextLabel.text
            Layout.fillWidth: true
            text: dialog.labelText
            onTextEdited: dialog.labelText = text
        }
        Label {
            id: labelPosLabel
            text: qsTr("Position")
        }
        MusicTimeSpinBox {
            id: labelPosSpinBox
            Accessible.labelledBy: labelPosLabel
            Accessible.name: labelPosLabel.text
            Layout.fillWidth: true
            timeline: dialog.timeline
            value: dialog.labelPos
            onValueModified: dialog.labelPos = value
        }
    }
    standardButtons: DialogButtonBox.Ok
}