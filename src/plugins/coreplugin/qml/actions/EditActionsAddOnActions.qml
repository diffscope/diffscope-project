import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.DspxModel as DspxModel
import DiffScope.DspxModel.SelectionModel as DspxSelectionModel
import DiffScope.DspxModel.PropertyMapper
import DiffScope.Core

ActionCollection {
    id: d

    required property QtObject addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property EditSourcesScenario editSourcesScenario: EditSourcesScenario {
        window: d.windowHandle?.window ?? null
        document: d.windowHandle?.projectDocumentContext.document ?? null
    }
    readonly property SourcesPickerModel sourcesPickerModel: SourcesPickerModel {
    }

    function selectedSingingClips() {
        const selectedItems = d.windowHandle?.projectDocumentContext.document
                               .selectionModel.clipSelectionModel.selectedItems ?? []
        const clips = []
        for (let index = 0; index < selectedItems.length; ++index) {
            if (selectedItems[index].type !== DspxModel.Clip.Singing)
                return []
            clips.push(selectedItems[index])
        }
        return clips
    }

    function editSelectedSingers() {
        const clips = selectedSingingClips()
        if (clips.length === 0)
            return
        sourcesPickerModel.fromSources(clips[0].sources ?? null)
        editSourcesScenario.editSources(sourcesPickerModel, clips)
    }

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
                return selectionType === DspxSelectionModel.SelectionModel.ST_Clip || selectionType === DspxSelectionModel.SelectionModel.ST_Track
            }
            checkable: true
            checked: {
                let selectionType = d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType
                if (selectionType === DspxSelectionModel.SelectionModel.ST_Track) {
                    return Boolean(trackPropertyMapper.mute)
                } else {
                    return Boolean(clipPropertyMapper.mute)
                }
                return false
            }
            onTriggered: () => {
                let selectionType = d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType
                if (selectionType === DspxSelectionModel.SelectionModel.ST_Track) {
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
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Track
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
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Track
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
            enabled: d.windowHandle?.projectDocumentContext.document.selectionModel.currentItem && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Track
            onTriggered: () => {
                let track = d.windowHandle.projectDocumentContext.document.selectionModel.currentItem;
                d.windowHandle.projectDocumentContext.document.selectionModel.select(null, DspxSelectionModel.SelectionModel.ClearPreviousSelection)
                for (let clip of track.clips.iterable) {
                    d.windowHandle.projectDocumentContext.document.selectionModel.select(clip, DspxSelectionModel.SelectionModel.Select)
                }
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftUpByASemitone"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Note
            onTriggered: d.addOn.shiftNotes(1)
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftDownByASemitone"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Note
            onTriggered: d.addOn.shiftNotes(-1)
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftUpByAnOctave"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Note
            onTriggered: d.addOn.shiftNotes(12)
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.shiftDownByAnOctave"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Note
            onTriggered: d.addOn.shiftNotes(-12)
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.selectionIndicator"
        Label {
            leftPadding: 4
            rightPadding: 4
            text: {
                const model = d.windowHandle?.projectDocumentContext.document.model
                const selectionModel = d.windowHandle?.projectDocumentContext.document.selectionModel
                if (!model || !selectionModel)
                    return ""
                const getContainerText = () => {
                    switch (selectionModel.selectionType) {
                    case DspxSelectionModel.SelectionModel.ST_None:
                        return qsTr("No selection")
                    case DspxSelectionModel.SelectionModel.ST_AnchorNode:
                        return "" // TODO
                    case DspxSelectionModel.SelectionModel.ST_Clip:
                        return qsTr("%Ln clip(s)", "", model.tracks.items.reduce((count, track) => count + track.clips.size, 0))
                    case DspxSelectionModel.SelectionModel.ST_Label:
                        return qsTr("%Ln label(s)", "", model.labels.size)
                    case DspxSelectionModel.SelectionModel.ST_Note:
                        return qsTr("%Ln note(s)", "", selectionModel.noteSelectionModel.noteSequenceWithSelectedItems?.size ?? 0)
                    case DspxSelectionModel.SelectionModel.ST_Tempo:
                        return qsTr("%Ln tempo(s)", "", model.tempos.size)
                    case DspxSelectionModel.SelectionModel.ST_Track:
                        return qsTr("%Ln track(s)", "", model.tracks.size)
                    case DspxSelectionModel.SelectionModel.ST_KeySignature:
                        return qsTr("%Ln key signature(s)", "", model.keySignatures.size)
                    default:
                        return ""
                    }
                }
                const getSelectionText = () => {
                    if (selectionModel.selectedCount === 0)
                        return ""
                    return qsTr(" (%Ln selected)", "", selectionModel.selectedCount)
                }
                return getContainerText() + getSelectionText()
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.split"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && (d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Clip || d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Note)
            onTriggered: d.windowHandle.projectDocumentContext.document.splitItems(d.windowHandle.projectTimeline.position)
        }
    }
    ActionItem {
        actionId: "org.diffscope.core.edit.bounceToClip"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Clip && d.windowHandle?.projectDocumentContext.document.selectionModel.clipSelectionModel.selectedSingingClipCount
            onTriggered: d.windowHandle.projectDocumentContext.document.bounceToClip()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.editCurrentClip"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Clip && d.windowHandle?.projectDocumentContext.document.selectionModel.currentItem?.type === DspxModel.Clip.Singing
            onTriggered: () => {
                let selectionModel = d.windowHandle.projectDocumentContext.document.selectionModel
                let clip = selectionModel.currentItem
                if (clip.notes !== selectionModel.noteSelectionModel.noteSequenceWithSelectedItems) {
                    selectionModel.select(null, DspxSelectionModel.SelectionModel.Select, DspxSelectionModel.SelectionModel.ST_Note, clip.notes)
                }
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.editSingers"
        Action {
            enabled: {
                const selectionModel = d.windowHandle?.projectDocumentContext.document.selectionModel
                if (!selectionModel || selectionModel.selectionType !== DspxSelectionModel.SelectionModel.ST_Clip)
                    return false
                const clipSelectionModel = selectionModel.clipSelectionModel
                return clipSelectionModel.selectedCount > 0
                       && clipSelectionModel.selectedSingingClipCount === clipSelectionModel.selectedCount
            }
            onTriggered: Qt.callLater(() => d.editSelectedSingers())
        }
    }
}
