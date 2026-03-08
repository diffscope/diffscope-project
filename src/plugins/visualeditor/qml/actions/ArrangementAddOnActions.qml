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
        actionId: "org.diffscope.visualeditor.arrangementPanel.scissorTool"
        Action {
            checkable: true
            checked: d.arrangementPanelInterface?.tool === ArrangementPanelInterface.ScissorTool
            onTriggered: () => {
                d.arrangementPanelInterface.tool = ArrangementPanelInterface.ScissorTool
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.arrangementPanel.selectTool"
        Action {
            checkable: true
            checked: d.arrangementPanelInterface?.tool === ArrangementPanelInterface.SelectTool
            icon.source: d.addOn?.altPressed ? "image://fluent-system-icons/cursor_text_rectangle_landscape_dash" : "image://fluent-system-icons/rectangle_landscape_dash"
            onTriggered: () => {
                d.arrangementPanelInterface.tool = ArrangementPanelInterface.SelectTool
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
            enabled: !d.arrangementPanelInterface?.snapTemporarilyDisabled
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.arrangementPanel.snapDuration"
        Menu {
            id: menu
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
                        checked: d.arrangementPanelInterface?.positionAlignmentManipulator.duration === modelData.data
                        onTriggered: () => {
                            d.arrangementPanelInterface.positionAlignmentManipulator.duration = modelData.data
                            Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
                        }
                    }
                }
                onObjectAdded: (index, object) => {
                    menu.insertAction(index, object)
                }
                onObjectRemoved: (index, object) => {
                    menu.removeAction(object)
                }
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.arrangementPanel.snapTuplet"
        Menu {
            id: menu
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
                        checked: d.arrangementPanelInterface?.positionAlignmentManipulator.tuplet === modelData.data
                        onTriggered: () => {
                            d.arrangementPanelInterface.positionAlignmentManipulator.tuplet = modelData.data
                            Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
                        }
                    }
                }
                onObjectAdded: (index, object) => {
                    menu.insertAction(index, object)
                }
                onObjectRemoved: (index, object) => {
                    menu.removeAction(object)
                }
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.arrangementPanel.autoPageScrolling"
        Action {
            checkable: true
            checked: d.arrangementPanelInterface?.autoPageScrollingManipulator.enabled ?? false
            onTriggered: d.arrangementPanelInterface.autoPageScrollingManipulator.enabled = checked
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.arrangementPanel.additionalTracks"
        Menu {
            id: menu
            Instantiator {
                model: d.addOn?.additionalTrackLoader.components ?? null
                delegate: Action {
                    required property string modelData
                    text: d.addOn.additionalTrackLoader.componentName(modelData)
                    checkable: true
                    checked: d.addOn.additionalTrackLoader.loadedComponents.indexOf(modelData) >= 0
                    onTriggered: () => {
                        if (checked) {
                            d.addOn.additionalTrackLoader.loadItem(modelData)
                        } else {
                            d.addOn.additionalTrackLoader.removeItem(modelData)
                        }
                    }
                }
                onObjectAdded: (index, object) => {
                    menu.insertAction(index, object)
                }
                onObjectRemoved: (index, object) => {
                    menu.removeAction(object)
                }
            }
        }
    }

}