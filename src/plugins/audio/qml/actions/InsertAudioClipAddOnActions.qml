import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.Audio

ActionCollection {
    id: d

    required property InsertAudioClipAddOn addOn

    ActionItem {
        actionId: "org.diffscope.audio.insert.insertAudioClip"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.insertAudioClip())
        }
    }
}
