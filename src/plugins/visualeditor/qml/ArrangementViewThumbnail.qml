// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick

import dev.sjimo.ScopicFlow

Item {
    id: thumbnail

    required property ClipViewModel clipViewModel
    required property bool visualVisible
    required property ArrangementPanelInterface arrangementPanelInterface
    required property PianoRollPanelInterface pianoRollPanelInterface
    required property Item viewport
    required property real trackTop

    readonly property rect visibleRect: {
        if (!visualVisible || !viewport || !clipViewModel || width <= 0 || height <= 0)
            return Qt.rect(0, 0, 0, 0)

        const viewportOffset = arrangementPanelInterface?.trackListLayoutViewModel?.viewportOffset ?? 0
        if (trackTop - viewportOffset >= viewport.height)
            return Qt.rect(0, 0, 0, 0)

        const start = arrangementPanelInterface?.timeViewModel?.start ?? 0
        const pixelDensity = arrangementPanelInterface?.timeLayoutViewModel?.pixelDensity ?? 0
        const clipX = (clipViewModel.position - start) * pixelDensity
        const clipY = mapToItem(viewport, 0, 0).y
        const left = Math.max(0, -clipX)
        const right = Math.min(width, viewport.width - clipX)
        const top = Math.max(0, -clipY)
        const bottom = Math.min(height, viewport.height - clipY)
        return Qt.rect(left, top, Math.max(0, right - left), Math.max(0, bottom - top))
    }

    ViewLayerHost {
        anchors.fill: parent
        point: ViewLayer.ArrangementClipContent
        arrangementPanelInterface: thumbnail.arrangementPanelInterface
        pianoRollPanelInterface: thumbnail.pianoRollPanelInterface
        clipViewModel: thumbnail.clipViewModel
        visibleRect: thumbnail.visibleRect
    }

    ViewLayerHost {
        anchors.fill: parent
        point: ViewLayer.ArrangementClipAdornment
        arrangementPanelInterface: thumbnail.arrangementPanelInterface
        pianoRollPanelInterface: thumbnail.pianoRollPanelInterface
        clipViewModel: thumbnail.clipViewModel
        visibleRect: thumbnail.visibleRect
    }
}
