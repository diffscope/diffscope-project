// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick

import dev.sjimo.ScopicFlow.Views as ScopicFlowViews

Item {
    id: layer

    readonly property var projectViewModelContext:
        ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
    readonly property var pianoRollPanelInterface:
        ViewLayer.pianoRollPanelInterface
    readonly property var editingClip:
        pianoRollPanelInterface?.editingClip ?? null
    readonly property var clipViewModel:
        projectViewModelContext?.getClipViewItemFromDocumentItem(editingClip) ?? null
    readonly property var timeViewModel:
        pianoRollPanelInterface?.timeViewModel ?? null
    readonly property var timeLayoutViewModel:
        pianoRollPanelInterface?.timeLayoutViewModel ?? null

    ScopicFlowViews.SequenceBoundaryView {
        anchors.fill: parent
        visible: (EditorPreference.parameterPanelOverlay
            & EditorPreference.POF_AllPhonemes) !== 0
        sequenceViewModel: layer.projectViewModelContext?.getPhonemeSequenceViewModel(
            layer.editingClip) ?? null
        timeViewModel: layer.timeViewModel
        timeLayoutViewModel: layer.timeLayoutViewModel
        clipViewModel: layer.clipViewModel
    }

    ScopicFlowViews.SequenceBoundaryView {
        anchors.fill: parent
        visible: (EditorPreference.parameterPanelOverlay
            & EditorPreference.POF_NoteBoundary) !== 0
        sequenceViewModel: layer.projectViewModelContext?.getNoteSequenceViewModel(
            layer.editingClip) ?? null
        timeViewModel: layer.timeViewModel
        timeLayoutViewModel: layer.timeLayoutViewModel
        clipViewModel: layer.clipViewModel
    }
}
