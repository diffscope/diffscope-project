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
            text: qsTr("Start position")
        }
        MusicTimeSpinBox {
            Layout.fillWidth: true
            timeline: dialog.timeline
            value: dialog.startPosition
            to: dialog.endPosition - 1
            onValueModified: dialog.startPosition = value
        }
        Label {
            text: qsTr("End position")
        }
        MusicTimeSpinBox {
            Layout.fillWidth: true
            timeline: dialog.timeline
            value: dialog.endPosition
            from: dialog.startPosition + 1
            onValueModified: dialog.endPosition = value
        }
    }
    standardButtons: DialogButtonBox.Ok
}