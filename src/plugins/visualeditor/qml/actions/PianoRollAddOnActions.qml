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
    readonly property PianoRollPanelInterface pianoRollPanelInterface: addOn?.pianoRollPanelInterface ?? null

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.pointerTool"
        Action {
            checkable: true
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.PointerTool
            onTriggered: () => {
                d.pianoRollPanelInterface.tool = PianoRollPanelInterface.PointerTool
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.pencilTool"
        Action {
            checkable: true
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.PencilTool
            onTriggered: () => {
                d.pianoRollPanelInterface.tool = PianoRollPanelInterface.PencilTool
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.selectTool"
        Action {
            checkable: true
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.SelectTool
            icon.source: d.addOn?.altPressed ? "image://fluent-system-icons/cursor_text_rectangle_landscape_dash" : "image://fluent-system-icons/rectangle_landscape_dash"
            onTriggered: () => {
                d.pianoRollPanelInterface.tool = PianoRollPanelInterface.SelectTool
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.handTool"
        Action {
            checkable: true
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.HandTool
            onTriggered: () => {
                d.pianoRollPanelInterface.tool = PianoRollPanelInterface.HandTool
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.snap"
        SnapControl {
            positionAlignmentManipulator: d.pianoRollPanelInterface?.positionAlignmentManipulator ?? null
            enabled: !d.pianoRollPanelInterface?.snapTemporarilyDisabled
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.snapDuration"
        Menu {
            id: menuDuration
            Instantiator {
                model: DelegateModel {
                    model: [
                        { text: qsTr("Auto"), data: PositionAlignmentManipulator.Auto },
                        { text: qsTr("None"), data: PositionAlignmentManipulator.Unset },
                        { text: qsTr("Whole note"), data: PositionAlignmentManipulator.Note1st },
                        { text: qsTr("Half note"), data: PositionAlignmentManipulator.Note2nd },
                        { text: qsTr("Quarter note"), data: PositionAlignmentManipulator.Note4th },
                        { text: qsTr("8th note"), data: PositionAlignmentManipulator.Note8th },
                        { text: qsTr("16th note"), data: PositionAlignmentManipulator.Note16th },
                        { text: qsTr("32nd note"), data: PositionAlignmentManipulator.Note32nd },
                        { text: qsTr("64th note"), data: PositionAlignmentManipulator.Note64th },
                        { text: qsTr("128th note"), data: PositionAlignmentManipulator.Note128th },
                    ]
                    delegate: Action {
                        required property var modelData
                        text: modelData.text
                        checkable: true
                        checked: d.pianoRollPanelInterface?.positionAlignmentManipulator.duration === modelData.data
                        onTriggered: () => {
                            d.pianoRollPanelInterface.positionAlignmentManipulator.duration = modelData.data
                            Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
                        }
                    }
                }
                onObjectAdded: (index, object) => {
                    menuDuration.insertAction(index, object)
                }
                onObjectRemoved: (index, object) => {
                    menuDuration.removeAction(object)
                }
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.snapTuplet"
        Menu {
            id: menuTuplet
            Instantiator {
                model: DelegateModel {
                    model: [
                        { text: qsTr("None"), data: PositionAlignmentManipulator.None },
                        { text: qsTr("Triplet"), data: PositionAlignmentManipulator.Triplet },
                        { text: qsTr("Quintuplet"), data: PositionAlignmentManipulator.Quintuplet },
                    ]
                    delegate: Action {
                        required property var modelData
                        text: modelData.text
                        checkable: true
                        checked: d.pianoRollPanelInterface?.positionAlignmentManipulator.tuplet === modelData.data
                        onTriggered: () => {
                            d.pianoRollPanelInterface.positionAlignmentManipulator.tuplet = modelData.data
                            Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
                        }
                    }
                }
                onObjectAdded: (index, object) => {
                    menuTuplet.insertAction(index, object)
                }
                onObjectRemoved: (index, object) => {
                    menuTuplet.removeAction(object)
                }
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.autoPageScrolling"
        Action {
            checkable: true
            checked: d.pianoRollPanelInterface?.autoPageScrollingManipulator.enabled ?? false
            onTriggered: d.pianoRollPanelInterface.autoPageScrollingManipulator.enabled = checked
        }
    }
}
