import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

ActionCollection {
    id: d

    required property QtObject addOn
    readonly property ArrangementPanelInterface arrangementPanelInterface: addOn?.arrangementPanelInterface ?? null

    ActionItem {
        actionId: "org.diffscope.visualeditor.arrangementPanel.pointerTool"
        Action {
            checkable: true
            checked: d.arrangementPanelInterface?.tool === ArrangementPanelInterface.PointerTool
            onTriggered: () => {
                d.arrangementPanelInterface.tool = ArrangementPanelInterface.PointerTool
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.arrangementPanel.pencilTool"
        Action {
            checkable: true
            checked: d.arrangementPanelInterface?.tool === ArrangementPanelInterface.PencilTool
            onTriggered: () => {
                d.arrangementPanelInterface.tool = ArrangementPanelInterface.PencilTool
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.arrangementPanel.handTool"
        Action {
            checkable: true
            checked: d.arrangementPanelInterface?.tool === ArrangementPanelInterface.HandTool
            onTriggered: () => {
                d.arrangementPanelInterface.tool = ArrangementPanelInterface.HandTool
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.arrangementPanel.snap"
        SnapControl {
            positionAlignmentManipulator: d.arrangementPanelInterface?.positionAlignmentManipulator ?? null
        }
    }

}