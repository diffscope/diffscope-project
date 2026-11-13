// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick

import dev.sjimo.ScopicFlow

import SVSCraft.UIComponents

import DiffScope.DspxModel as DspxModel
import DiffScope.VisualEditor

Loader {
    id: thumbnailLoader

    readonly property ClipViewModel clipViewModel: ViewLayer.clipViewModel
    readonly property ProjectViewModelContext projectViewModelContext: ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
    readonly property DspxModel.AudioClip audioClip: {
        const clip = clipViewModel ? projectViewModelContext?.getClipDocumentItemFromViewItem(clipViewModel) : null
        return clip?.kind === DspxModel.Clip.Audio ? clip : null
    }
    readonly property rect visibleClipRect: ViewLayer.visibleRect

    active: audioClip !== null && visibleClipRect.width > 0 && visibleClipRect.height > 0

    sourceComponent: AudioThumbnailWaveformThumbnail {
        id: d

        readonly property real ticksPerPixel: width > 0 ? (thumbnailLoader.clipViewModel?.length ?? 0) / width : 0

        audioClip: thumbnailLoader.audioClip
        projectWindowInterface: thumbnailLoader.ViewLayer.windowHandle
        viewportOffset: thumbnailLoader.visibleClipRect.x * ticksPerPixel
        viewportLength: thumbnailLoader.visibleClipRect.width * ticksPerPixel
        color: Theme.foregroundPrimaryColor
    }
}
