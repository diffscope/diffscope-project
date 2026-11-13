// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

import dev.sjimo.ScopicFlow

import SVSCraft.UIComponents

import DiffScope.VisualEditor

Loader {
    id: adornment

    readonly property ClipViewModel clipViewModel: ViewLayer.clipViewModel
    readonly property rect viewport: ViewLayer.visibleRect

    z: -1
    active: SynthesisVisualizerSettings.showClipWaveforms && Boolean(clipViewModel?.associatedNoteSequence)
            && viewport.width > 0 && viewport.height > 0

    sourceComponent: SynthesisWaveformLayer {
        projectViewModelContext: adornment.ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
        clipViewModel: adornment.clipViewModel
        startTick: adornment.clipViewModel?.position ?? 0
        pixelDensity: adornment.clipViewModel?.length > 0 ? adornment.width / adornment.clipViewModel.length : 0
        viewport: adornment.viewport
        foregroundColor: Theme.foregroundPrimaryColor
        normalizeAmplitude: false
        waveformOpacity: 0.5
    }
}
