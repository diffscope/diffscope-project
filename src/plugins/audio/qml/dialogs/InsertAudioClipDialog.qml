import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core
import DiffScope.DspxModel as DspxModel

Dialog {
    id: dialog

    property DspxModel.TrackList trackList
    property DspxModel.Track selectedTrack
    property MusicTimeline timeline
    property int clipPosition
    property string clipName

    title: qsTr("Insert Audio Clip")

    onAboutToShow: trackCombo.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2

        Label {
            id: trackLabel
            text: qsTr("Track")
        }
        ComboBox {
            id: trackCombo
            Accessible.labelledBy: trackLabel
            Accessible.name: trackLabel.text
            Layout.fillWidth: true
            model: dialog.trackList?.items.map((track, i) => ({ text: qsTr("%L1: %2").arg(i + 1).arg(track.name), value: track })) ?? null
            textRole: "text"
            valueRole: "value"
            currentValue: dialog.selectedTrack
            onActivated: (index) => {
                const value = valueAt(index)
                if (dialog.selectedTrack === value)
                    return
                dialog.selectedTrack = value
            }
        }

        Label {
            id: positionLabel
            text: qsTr("Position")
        }
        MusicTimeSpinBox {
            id: positionSpinBox
            Accessible.labelledBy: positionLabel
            Accessible.name: positionLabel.text
            Layout.fillWidth: true
            timeline: dialog.timeline
            value: dialog.clipPosition
            onValueModified: dialog.clipPosition = value
        }

        Label {
            id: nameLabel
            text: qsTr("Name")
        }
        TextField {
            id: nameTextField
            Accessible.labelledBy: nameLabel
            Accessible.name: nameLabel.text
            Layout.fillWidth: true
            text: dialog.clipName
            onTextEdited: dialog.clipName = text
        }
    }

    standardButtons: DialogButtonBox.Ok
}
