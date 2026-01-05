import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

ActionCollection {
    id: d

    required property QtObject addOn
    readonly property MixerPanelInterface mixerPanelInterface: addOn?.mixerPanelInterface ?? null

    ActionItem {
        actionId: "org.diffscope.visualeditor.mixerPanel.pointerTool"
        Action {
            checkable: true
            checked: d.mixerPanelInterface?.tool === MixerPanelInterface.PointerTool
            onTriggered: () => {
                d.mixerPanelInterface.tool = MixerPanelInterface.PointerTool
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.mixerPanel.handTool"
        Action {
            checkable: true
            checked: d.mixerPanelInterface?.tool === MixerPanelInterface.HandTool
            onTriggered: () => {
                d.mixerPanelInterface.tool = MixerPanelInterface.HandTool
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }
}
