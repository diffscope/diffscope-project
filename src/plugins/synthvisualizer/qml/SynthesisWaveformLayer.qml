// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

import dev.sjimo.ScopicFlow

import DiffScope.DspxModel as DspxModel
import DiffScope.VisualEditor

Item {
    id: waveformLayer

    required property ProjectViewModelContext projectViewModelContext
    required property ClipViewModel clipViewModel
    required property real startTick
    required property real pixelDensity
    required property color foregroundColor
    required property bool normalizeAmplitude
    required property real waveformOpacity
    property rect viewport: Qt.rect(0, 0, width, height)

    readonly property var documentClip: projectViewModelContext?.getClipDocumentItemFromViewItem(clipViewModel) ?? null
    readonly property DspxModel.SingingClip singingClip: documentClip?.kind === DspxModel.Clip.Singing ? documentClip : null
    readonly property var trackViewModel: projectViewModelContext?.getTrackViewItemFromDocumentItem(
        singingClip?.clipSequence?.track ?? null) ?? null

    clip: true

    SynthesisPieceModel {
        id: pieceModel

        windowHandle: waveformLayer.projectViewModelContext?.windowHandle ?? null
        singingClip: waveformLayer.singingClip
    }

    Repeater {
        model: pieceModel

        delegate: SynthesisWaveformThumbnail {
            id: waveform

            required property real absolutePosition
            required property real duration
            required property bool ready
            required property string audioFilePath

            x: (absolutePosition - waveformLayer.startTick) * waveformLayer.pixelDensity
            width: duration * waveformLayer.pixelDensity
            y: (waveformLayer.height - height) / 2
            height: waveformLayer.height * (waveformLayer.normalizeAmplitude ? verticalScaleFactor : 1)
            visible: ready && audioFilePath.length > 0 && width > 0
                     && waveformLayer.viewport.width > 0 && waveformLayer.viewport.height > 0
                     && x < waveformLayer.viewport.x + waveformLayer.viewport.width
                     && x + width > waveformLayer.viewport.x

            projectWindowInterface: waveformLayer.projectViewModelContext?.windowHandle ?? null
            sourceFilePath: ready ? audioFilePath : ""
            startTick: absolutePosition
            durationTicks: duration
            color: waveformLayer.foregroundColor
            opacity: waveformLayer.waveformOpacity
        }
    }
}
