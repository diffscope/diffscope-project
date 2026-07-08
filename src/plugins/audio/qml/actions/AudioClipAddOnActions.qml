import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.Audio
import DiffScope.Core
import DiffScope.DspxModel as DspxModel
import DiffScope.DspxModel.SelectionModel as DspxSelectionModel

ActionCollection {
    id: d

    required property AudioClipAddOn addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null

    ActionItem {
        actionId: "org.diffscope.audio.insert.insertAudioClip"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.insertAudioClip())
        }
    }

    ActionItem {
        actionId: "org.diffscope.audio.edit.replaceAudioClip"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Clip
                     && d.windowHandle?.projectDocumentContext.document.selectionModel.currentItem?.type === DspxModel.Clip.Audio
            onTriggered: Qt.callLater(() => d.addOn.replaceAudioClip(d.windowHandle.projectDocumentContext.document.selectionModel.currentItem))
        }
    }
}
