// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQuick

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow

import DiffScope.Core
import DiffScope.DspxModel as DspxModel
import DiffScope.VisualEditor

Item {
    id: control

    required property ClipViewModel clipViewModel
    required property ProjectViewModelContext projectViewModelContext
    required property TimeViewModel timeViewModel
    required property TimeLayoutViewModel timeLayoutViewModel

    readonly property DspxModel.SingingClip singingClip:
        projectViewModelContext?.getClipDocumentItemFromViewItem(
            clipViewModel) ?? null

    clip: true

    SynthesisPieceModel {
        id: pieceModel

        windowHandle: control.projectViewModelContext?.windowHandle ?? null
        singingClip: control.singingClip
    }

    Repeater {
        model: pieceModel

        delegate: SynthesisWaveformThumbnail {
            id: waveformDelegate

            required property real absolutePosition
            required property real duration
            required property bool ready
            required property string audioFilePath

            x: (absolutePosition - (control.timeViewModel?.start ?? 0))
               * (control.timeLayoutViewModel?.pixelDensity ?? 0)
            width: duration
                   * (control.timeLayoutViewModel?.pixelDensity ?? 0)
            y: (control.height - height) / 2
            height: control.height * verticalScaleFactor
            visible: ready && audioFilePath.length > 0
                     && width > 0 && x < control.width && x + width > 0

            projectWindowInterface:
                control.projectViewModelContext?.windowHandle ?? null
            sourceFilePath: audioFilePath
            startTick: absolutePosition
            durationTicks: duration
            color: Theme.accentColor
            opacity: 0.25
        }
    }
}
