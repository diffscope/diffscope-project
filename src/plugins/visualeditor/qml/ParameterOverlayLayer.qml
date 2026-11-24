// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick

import dev.sjimo.ScopicFlow.Views as ScopicFlowViews

ScopicFlowViews.SequenceBoundaryView {
    readonly property var projectViewModelContext:
        ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
    readonly property var pianoRollPanelInterface:
        ViewLayer.pianoRollPanelInterface
    readonly property var editingClip:
        pianoRollPanelInterface?.editingClip ?? null
    readonly property int overlayMode:
        EditorPreference.parameterPanelOverlayMode

    visible: overlayMode !== EditorPreference.POM_None
    sequenceViewModel: {
        switch (overlayMode) {
        case EditorPreference.POM_FirstPhoneme:
            return projectViewModelContext?.getFirstPhonemeSequenceViewModel(editingClip) ?? null
        case EditorPreference.POM_NoteBoundary:
            return projectViewModelContext?.getNoteSequenceViewModel(editingClip) ?? null
        default:
            return null
        }
    }
    timeViewModel: pianoRollPanelInterface?.timeViewModel ?? null
    timeLayoutViewModel: pianoRollPanelInterface?.timeLayoutViewModel ?? null
    clipViewModel: projectViewModelContext?.getClipViewItemFromDocumentItem(editingClip) ?? null
}
