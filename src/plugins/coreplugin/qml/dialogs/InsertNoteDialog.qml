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
    property int notePosition
    property int noteLength
    property int notePitch
    property string noteLyric

    title: qsTr("Insert Note")

    onAboutToShow: notePositionSpinBox.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2

        Label {
            id: notePositionLabel
            text: qsTr("Onset Position")
        }
        MusicTimeOffsetSpinBox {
            id: notePositionSpinBox
            Accessible.labelledBy: notePositionLabel
            Accessible.name: notePositionLabel.text
            Layout.fillWidth: true
            value: dialog.notePosition
            from: 0
            to: 2147483647
            onValueModified: dialog.notePosition = value
        }

        Label {
            id: noteLengthLabel
            text: qsTr("Length")
        }
        MusicTimeOffsetSpinBox {
            id: noteLengthSpinBox
            Accessible.labelledBy: noteLengthLabel
            Accessible.name: noteLengthLabel.text
            Layout.fillWidth: true
            value: Math.max(1, dialog.noteLength)
            from: 1
            to: 2147483647
            onValueModified: dialog.noteLength = value
        }

        Label {
            id: notePitchLabel
            text: qsTr("Pitch")
        }
        MusicPitchSpinBox {
            id: notePitchSpinBox
            Accessible.labelledBy: notePitchLabel
            Accessible.name: notePitchLabel.text
            Layout.fillWidth: true
            value: dialog.notePitch
            from: 0
            to: 127
            onValueModified: dialog.notePitch = value
        }

        Label {
            id: noteLyricLabel
            text: qsTr("Lyric")
        }
        TextField {
            id: noteLyricTextField
            Accessible.labelledBy: noteLyricLabel
            Accessible.name: noteLyricLabel.text
            Layout.fillWidth: true
            text: dialog.noteLyric
            onTextEdited: dialog.noteLyric = text
        }
    }

    standardButtons: DialogButtonBox.Ok
}
