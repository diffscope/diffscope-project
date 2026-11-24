// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

import SVSCraft.UIComponents

import DiffScope.VisualEditor

Loader {
    id: control

    readonly property var projectViewModelContext:
        control.ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
    readonly property var clipViewModel:
        projectViewModelContext?.getClipViewItemFromDocumentItem(
            control.ViewLayer.pianoRollPanelInterface?.editingClip ?? null) ?? null

    active: SynthesisVisualizerSettings.showPhonemeWaveforms

    sourceComponent: SynthesisWaveformLayer {
        id: waveforms

        projectViewModelContext: control.projectViewModelContext
        clipViewModel: control.clipViewModel
        startTick: control.ViewLayer.pianoRollPanelInterface?.timeViewModel?.start ?? 0
        pixelDensity: control.ViewLayer.pianoRollPanelInterface?.timeLayoutViewModel?.pixelDensity ?? 0
        foregroundColor: waveforms.trackViewModel?.color ?? Theme.accentColor
        normalizeAmplitude: true
        waveformOpacity: 0.25
    }
}
