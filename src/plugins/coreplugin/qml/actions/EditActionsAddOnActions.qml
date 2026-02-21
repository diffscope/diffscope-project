import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.DspxModel as DspxModel

ActionCollection {
    id: d

    required property QtObject addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null

    component EditAction: Action {
        required property int flag
        enabled: d.windowHandle.mainEditActionsHandlerRegistry.enabledActions & flag
        onTriggered: d.windowHandle.mainEditActionsHandlerRegistry.triggerEditAction(flag)
    }

    component MoveAction: Action {
        required property int direction
        enabled: d.windowHandle.mainEditActionsHandlerRegistry.enabledMoveDirections & direction
        onTriggered: d.windowHandle.mainEditActionsHandlerRegistry.move(direction, 0)
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.cut"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected ?? false
            onTriggered: d.windowHandle.projectDocumentContext.document.cutSelection(d.windowHandle.projectTimeline.position)
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.copy"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected ?? false
            onTriggered: d.windowHandle.projectDocumentContext.document.copySelection(d.windowHandle.projectTimeline.position)
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.paste"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.pasteAvailable ?? false
            onTriggered: d.windowHandle.projectDocumentContext.document.paste(d.windowHandle.projectTimeline.position)
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.delete"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected ?? false
            onTriggered: d.windowHandle.projectDocumentContext.document.deleteSelection()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.selectAll"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.editScopeFocused ?? false
            onTriggered: d.windowHandle.projectDocumentContext.document.selectAll()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.deselect"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected ?? false
            onTriggered: d.windowHandle.projectDocumentContext.document.deselectAll()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.selectCurrent"
        Action {
            enabled: Boolean(d.windowHandle?.projectDocumentContext.document.selectionModel.currentItem)
            onTriggered: d.addOn.selectCurrent()
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.multipleSelectCurrent"
        Action {
            enabled: Boolean(d.windowHandle?.projectDocumentContext.document.selectionModel.currentItem)
            onTriggered: d.addOn.multipleSelectCurrent()
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftCursorUp"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.editScopeFocused ?? false
            onTriggered: d.addOn.shiftCursor(EditActionsAddOn.ShiftCursorDirection_Up)
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftCursorDown"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.editScopeFocused ?? false
            onTriggered: d.addOn.shiftCursor(EditActionsAddOn.ShiftCursorDirection_Down)
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftCursorLeft"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.editScopeFocused ?? false
            onTriggered: d.addOn.shiftCursor(EditActionsAddOn.ShiftCursorDirection_Left)
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftCursorRight"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.editScopeFocused ?? false
            onTriggered: d.addOn.shiftCursor(EditActionsAddOn.ShiftCursorDirection_Right)
        }
    }
    readonly property TrackPropertyMapper trackPropertyMapper: TrackPropertyMapper {
        id: trackPropertyMapper
        selectionModel: d.windowHandle?.projectDocumentContext.document.selectionModel ?? null
    }
    readonly property ClipPropertyMapper clipPropertyMapper: ClipPropertyMapper {
        id: clipPropertyMapper
        selectionModel: d.windowHandle?.projectDocumentContext.document.selectionModel ?? null
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.mute"
        Action {
            enabled: {
                if (!d.windowHandle?.projectDocumentContext.document.anyItemsSelected)
                    return false
                let selectionType = d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType
                return selectionType === DspxModel.SelectionModel.ST_Clip || selectionType === DspxModel.SelectionModel.ST_Track
            }
            checkable: true
            checked: {
                let selectionType = d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType
                if (selectionType === DspxModel.SelectionModel.ST_Track) {
                    return Boolean(trackPropertyMapper.mute)
                } else {
                    return Boolean(clipPropertyMapper.mute)
                }
                return false
            }
            onTriggered: () => {
                let selectionType = d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType
                if (selectionType === DspxModel.SelectionModel.ST_Track) {
                    trackPropertyMapper.mute = checked
                } else {
                    clipPropertyMapper.mute = checked
                }
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.solo"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxModel.SelectionModel.ST_Track
            checkable: true
            checked: Boolean(trackPropertyMapper.solo)
            onTriggered: () => {
                trackPropertyMapper.solo = checked
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.record"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxModel.SelectionModel.ST_Track
            checkable: true
            checked: Boolean(trackPropertyMapper.record)
            onTriggered: () => {
                trackPropertyMapper.record = checked
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.selectAllClipsOnCurrentTrack"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.selectionModel.currentItem && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxModel.SelectionModel.ST_Track
            onTriggered: () => {
                let track = d.windowHandle.projectDocumentContext.document.selectionModel.currentItem;
                d.windowHandle.projectDocumentContext.document.selectionModel.select(null, DspxModel.SelectionModel.ClearPreviousSelection)
                for (let clip of track.clips.iterable) {
                    d.windowHandle.projectDocumentContext.document.selectionModel.select(clip, DspxModel.SelectionModel.Select)
                }
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftUpByASemitone"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxModel.SelectionModel.ST_Note
            onTriggered: d.addOn.shiftNotes(1)
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftDownByASemitone"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxModel.SelectionModel.ST_Note
            onTriggered: d.addOn.shiftNotes(-1)
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftUpByAnOctave"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxModel.SelectionModel.ST_Note
            onTriggered: d.addOn.shiftNotes(12)
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftDownByAnOctave"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxModel.SelectionModel.ST_Note
            onTriggered: d.addOn.shiftNotes(-12)
        }
    }

}