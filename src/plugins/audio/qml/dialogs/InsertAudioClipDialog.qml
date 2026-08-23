// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

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
    property QtObject player

    readonly property string currentTimeText: player ? formatTime(player.positionSecond) : "0:00"
    readonly property string totalTimeText: player ? formatTime(player.lengthSecond) : "0:00"

    function formatTime(seconds) {
        const totalSeconds = Math.max(0, Math.floor(seconds))
        const minutes = Math.floor(totalSeconds / 60)
        const secs = totalSeconds % 60
        return `${minutes}:${String(secs).padStart(2, '0')}`
    }

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

        RowLayout {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            spacing: 8
            enabled: dialog.player

            ToolButton {
                id: playButton
                checkable: true
                checked: dialog.player?.playing ?? false
                onClicked: if (dialog.player) dialog.player.playing = checked
                text: playButton.checked ? qsTr("Pause") : qsTr("Play")
                Accessible.name: text
                icon.source: playButton.checked ? "image://fluent-system-icons/pause" : "image://fluent-system-icons/play"
                display: AbstractButton.IconOnly
            }

            Slider {
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, dialog.player?.lengthSecond ?? 1)
                value: dialog.player?.positionSecond ?? 0
                onMoved: if (dialog.player) dialog.player.positionSecond = value
            }

            Label {
                text: qsTr("%1 / %2").arg(dialog.currentTimeText).arg(dialog.totalTimeText)
            }
        }
    }

    standardButtons: DialogButtonBox.Ok
}
