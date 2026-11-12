// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick

import dev.sjimo.ScopicFlow

import SVSCraft.UIComponents

Item {
    id: clipOutsideLayer
    readonly property PianoRollPanelInterface pianoRollPanelInterface: ViewLayer.pianoRollPanelInterface
    readonly property ProjectViewModelContext projectViewModelContext: ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null

    z: 1
    visible: Boolean(clipOutsideLayer.pianoRollPanelInterface?.editingClip)
    readonly property ClipViewModel clipViewModel: clipOutsideLayer.projectViewModelContext?.getClipViewItemFromDocumentItem(clipOutsideLayer.pianoRollPanelInterface?.editingClip ?? null) ?? null
    Rectangle {
        x: 0
        width: Math.min(parent.width, ((clipOutsideLayer.clipViewModel?.position ?? 0) - (clipOutsideLayer.pianoRollPanelInterface?.timeViewModel?.start ?? 0)) * (clipOutsideLayer.pianoRollPanelInterface?.timeLayoutViewModel?.pixelDensity ?? 0))
        height: parent.height
        color: Qt.rgba(Theme.backgroundPrimaryColor.r, Theme.backgroundPrimaryColor.g, Theme.backgroundPrimaryColor.b, 0.5 * Theme.backgroundPrimaryColor.a)
    }
    Rectangle {
        x: parent.width - width
        width: Math.min(parent.width, parent.width - ((clipOutsideLayer.clipViewModel?.position ?? 0) + (clipOutsideLayer.clipViewModel?.length ?? 0) - (clipOutsideLayer.pianoRollPanelInterface?.timeViewModel?.start ?? 0)) * (clipOutsideLayer.pianoRollPanelInterface?.timeLayoutViewModel?.pixelDensity ?? 0))
        height: parent.height
        color: Qt.rgba(Theme.backgroundPrimaryColor.r, Theme.backgroundPrimaryColor.g, Theme.backgroundPrimaryColor.b, 0.5 * Theme.backgroundPrimaryColor.a)
    }
}
