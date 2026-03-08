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
    property bool loopEnabled
    property int startPosition
    property int endPosition

    title: qsTr("Edit Loop")

    onAboutToShow: startPositionSpinBox.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2
        CheckBox {
            Layout.columnSpan: 2
            text: qsTr("Enable loop")
            checked: dialog.loopEnabled
            onClicked: dialog.loopEnabled = checked
        }
        Label {
            id: startPositionLabel
            text: qsTr("Start position")
        }
        MusicTimeSpinBox {
            id: startPositionSpinBox
            Accessible.labelledBy: startPositionLabel
            Accessible.name: startPositionLabel.text
            Layout.fillWidth: true
            timeline: dialog.timeline
            value: dialog.startPosition
            to: dialog.endPosition - 1
            onValueModified: dialog.startPosition = value
        }
        Label {
            id: endPositionLabel
            text: qsTr("End position")
        }
        MusicTimeSpinBox {
            id: endPositionSpinBox
            Accessible.labelledBy: endPositionLabel
            Accessible.name: endPositionLabel.text
            Layout.fillWidth: true
            timeline: dialog.timeline
            value: dialog.endPosition
            from: dialog.startPosition + 1
            onValueModified: dialog.endPosition = value
        }
    }
    standardButtons: DialogButtonBox.Ok
}