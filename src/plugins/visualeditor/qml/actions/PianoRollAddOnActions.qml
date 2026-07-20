import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.DspxModel as DspxModel
import DiffScope.DspxModel.SelectionModel as DspxSelectionModel

ActionCollection {
    id: d

    required property QtObject addOn
    readonly property PianoRollPanelInterface pianoRollPanelInterface: addOn?.pianoRollPanelInterface ?? null
    readonly property ActionGroup toolActionGroup: ActionGroup {
        exclusive: true
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.pointerTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.PointerTool
            onTriggered: () => {
                d.pianoRollPanelInterface.tool = PianoRollPanelInterface.PointerTool
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.pencilTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.PencilTool
            onTriggered: () => {
                d.pianoRollPanelInterface.tool = PianoRollPanelInterface.PencilTool
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.scissorTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.ScissorTool
            onTriggered: () => {
                d.pianoRollPanelInterface.tool = PianoRollPanelInterface.ScissorTool
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.selectTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.SelectTool
            icon.source: d.addOn?.altPressed ? "image://fluent-system-icons/cursor_text_rectangle_landscape_dash" : "image://fluent-system-icons/rectangle_landscape_dash"
            onTriggered: () => {
                d.pianoRollPanelInterface.tool = PianoRollPanelInterface.SelectTool
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.handTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.HandTool
            onTriggered: () => {
                d.pianoRollPanelInterface.tool = PianoRollPanelInterface.HandTool
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.pitchPencilTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.PitchPencilTool
            onTriggered: d.pianoRollPanelInterface.tool = PianoRollPanelInterface.PitchPencilTool
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.pitchEraserTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.PitchEraserTool
            onTriggered: d.pianoRollPanelInterface.tool = PianoRollPanelInterface.PitchEraserTool
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.pitchRangeSelectTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.PitchRangeSelectTool
            onTriggered: d.pianoRollPanelInterface.tool = PianoRollPanelInterface.PitchRangeSelectTool
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.pitchPointerTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.PitchPointerTool
            onTriggered: d.pianoRollPanelInterface.tool = PianoRollPanelInterface.PitchPointerTool
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.pitchPenTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.PitchPenTool
            onTriggered: d.pianoRollPanelInterface.tool = PianoRollPanelInterface.PitchPenTool
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.pitchConvertTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.PitchConvertTool
            onTriggered: d.pianoRollPanelInterface.tool = PianoRollPanelInterface.PitchConvertTool
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.pitchAnchorSelectTool"
        Action {
            checkable: true
            ActionGroup.group: d.toolActionGroup
            checked: d.pianoRollPanelInterface?.tool === PianoRollPanelInterface.PitchAnchorSelectTool
            onTriggered: d.pianoRollPanelInterface.tool = PianoRollPanelInterface.PitchAnchorSelectTool
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
        actionId: "org.diffscope.visualeditor.pianoRollPanel.editingClip"
        Item {
            id: editClipControl

            readonly property var selectorModel: d.pianoRollPanelInterface?.editingClipSelectorModel ?? null

            implicitWidth: layout.implicitWidth
            implicitHeight: layout.implicitHeight
            enabled: !!d.pianoRollPanelInterface?.editingClip

            RowLayout {
                id: layout

                ComboBox {
                    implicitHeight: 24

                    DescriptiveText.toolTip: qsTr("Edit clip")
                    DescriptiveText.activated: hovered

                    model: editClipControl.selectorModel
                    textRole: "display"
                    valueRole: "clip"

                    Component.onCompleted: currentValue = Qt.binding(() => {
                        return d.pianoRollPanelInterface?.editingClip ?? null
                    })
                    onActivated: (index) => {
                        if (!d.pianoRollPanelInterface || !model) {
                            return
                        }

                        const value = valueAt(index)
                        if (value !== undefined) {
                            d.pianoRollPanelInterface.editingClip = value
                            d.addOn.windowHandle.projectDocumentContext.document.selectionModel.select(null, DspxSelectionModel.SelectionModel.Select, DspxSelectionModel.SelectionModel.ST_Note, value.notes);
                        }
                    }
                }
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.highlightScale"
        Action {
            checkable: true
            checked: d.pianoRollPanelInterface?.scaleHighlightEnabled ?? false
            onTriggered: d.pianoRollPanelInterface.scaleHighlightEnabled = checked
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

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.showTrackSelector"
        Action {
            checkable: true
            checked: d.addOn?.trackSelectorVisible ?? false
            onTriggered: d.addOn.trackSelectorVisible = checked
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.additionalTracks"
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

    ActionItem {
        actionId: "org.diffscope.visualeditor.pianoRollPanel.bottomAdditionalTracks"
        Menu {
            id: bottomMenu
            Instantiator {
                model: d.addOn?.bottomAdditionalTrackLoader.components ?? null
                delegate: Action {
                    required property string modelData
                    text: d.addOn.bottomAdditionalTrackLoader.componentName(modelData)
                    checkable: true
                    checked: d.addOn.bottomAdditionalTrackLoader.loadedComponents.indexOf(modelData) >= 0
                    onTriggered: () => {
                        if (checked) {
                            d.addOn.bottomAdditionalTrackLoader.loadItem(modelData)
                        } else {
                            d.addOn.bottomAdditionalTrackLoader.removeItem(modelData)
                        }
                    }
                }
                onObjectAdded: (index, object) => {
                    bottomMenu.insertAction(index, object)
                }
                onObjectRemoved: (index, object) => {
                    bottomMenu.removeAction(object)
                }
            }
        }
    }
}
