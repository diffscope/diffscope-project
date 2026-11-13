// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQml.Models
import QtQuick

import dev.sjimo.ScopicFlow

import DiffScope.Core
import DiffScope.DspxModel as DspxModel

Item {
    id: layer

    readonly property PianoRollPanelInterface pianoRoll: ViewLayer.pianoRollPanelInterface
    readonly property ProjectViewModelContext context: ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
    readonly property rect viewport: ViewLayer.visibleRect
    readonly property real timeStart: pianoRoll?.timeViewModel.start ?? 0
    readonly property real timeScale: pianoRoll?.timeLayoutViewModel.pixelDensity ?? 0
    readonly property real keyStart: pianoRoll?.clavierViewModel.start ?? 0
    readonly property real keyHeight: pianoRoll?.clavierViewModel.pixelDensity ?? 0
    property var trackOverlays: []
    readonly property DocumentProblems documentProblems: problemQuery

    readonly property var noteProblems: {
        if (!pianoRoll?.editingClip || !context)
            return []

        const editingClip = pianoRoll.editingClip
        const editingTrack = editingClip.clipSequence?.track
        const visibleTracks = new Set(trackOverlays.filter(overlay => overlay.overlayVisible).map(overlay => overlay.track))
        const notes = new Map()
        for (const problem of problemQuery.problems) {
            const note = problem.entity
            if (!(note instanceof DspxModel.Note))
                continue
            const clip = note.noteSequence?.singingClip
            const track = clip?.clipSequence?.track
            if (!track || (track !== editingTrack && !visibleTracks.has(track)))
                continue
            if (notes.has(note)) {
                notes.get(note).problems.push(problem)
                continue
            }
            const clipModel = context.getClipViewItemFromDocumentItem(clip)
            const noteModel = context.getNoteViewItemFromDocumentItem(note)
            if (!clipModel || !noteModel)
                continue
            notes.set(note, {
                clipModel,
                noteModel,
                editing: clip === editingClip,
                documentProblems: problemQuery,
                problems: [problem]
            })
        }
        const groups = Array.from(notes.values())
        for (const group of groups)
            group.problems.sort((left, right) => left.severity - right.severity)
        return groups
    }

    function noteStartTick(note: var): double {
        return note.clipModel && note.noteModel ? note.clipModel.position - note.clipModel.clipStart + note.noteModel.position : 0
    }

    function noteRect(note: var): rect {
        if (!note.clipModel || !note.noteModel)
            return Qt.rect(0, 0, 0, 0)
        const x = (noteStartTick(note) - timeStart) * timeScale
        const y = (keyStart - note.noteModel.key - 1) * keyHeight
        const bodyHeight = note.editing ? Math.max(0, keyHeight - 1) : 4
        return Qt.rect(x + 0.5, y + (keyHeight - bodyHeight) / 2,
            Math.max(0, note.noteModel.length * timeScale - 1), bodyHeight)
    }

    Instantiator {
        model: DelegateModel {
            model: layer.pianoRoll?.trackOverlaySelectorModel ?? null
            delegate: QtObject {
                required property var display
                readonly property var track: display?.track ?? null
                readonly property bool overlayVisible: display?.overlayVisible ?? false
            }
        }
        onObjectAdded: (_, overlay) => layer.trackOverlays = layer.trackOverlays.concat([overlay])
        onObjectRemoved: (_, overlay) => layer.trackOverlays = layer.trackOverlays.filter(item => item !== overlay)
    }

    DocumentProblems {
        id: problemQuery
        windowHandle: layer.ViewLayer.windowHandle
        entity: problemQuery.windowHandle?.projectDocumentContext.document.model ?? null
    }
}
