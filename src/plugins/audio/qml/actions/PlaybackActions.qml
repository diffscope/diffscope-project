import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Audio

ActionCollection {
    id: d

    required property PlaybackAddOn addOn

    ActionItem {
        actionId: "org.diffscope.audio.playback.play"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.play())
        }
    }

    ActionItem {
        actionId: "org.diffscope.audio.playback.pause"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.pause())
        }
    }

    ActionItem {
        actionId: "org.diffscope.audio.playback.stop"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.stop())
        }
    }

    ActionItem {
        actionId: "org.diffscope.audio.playback.toggle"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.togglePlayback())
        }
    }

    ActionItem {
        actionId: "org.diffscope.audio.playback.playButton"
        ToolButton {
            action: d.addOn?.getPlayAction() ?? null
            display: AbstractButton.IconOnly
            highlighted: d.addOn?.projectAudioContext.status === 1
            ThemedItem.controlType: SVS.CT_Accent
        }
    }

    ActionItem {
        actionId: "org.diffscope.audio.playback.pauseButton"
        ToolButton {
            action: d.addOn?.getPauseAction() ?? null
            display: AbstractButton.IconOnly
            highlighted: d.addOn?.projectAudioContext.status === 2
            ThemedItem.controlType: SVS.CT_Warning
        }
    }

}
