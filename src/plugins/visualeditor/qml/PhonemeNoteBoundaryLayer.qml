// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick

import dev.sjimo.ScopicFlow.Views as ScopicFlowViews

ScopicFlowViews.SequenceBoundaryView {
    readonly property var projectViewModelContext:
        ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
    readonly property var pianoRollPanelInterface:
        ViewLayer.pianoRollPanelInterface

    visible: EditorPreference.showPhonemeNoteBoundaryOverlay
    sequenceViewModel: projectViewModelContext?.getNoteSequenceViewModel(
        pianoRollPanelInterface?.editingClip ?? null) ?? null
    timeViewModel: pianoRollPanelInterface?.timeViewModel ?? null
    timeLayoutViewModel: pianoRollPanelInterface?.timeLayoutViewModel ?? null
    clipViewModel: projectViewModelContext?.getClipViewItemFromDocumentItem(
        pianoRollPanelInterface?.editingClip ?? null) ?? null
}
