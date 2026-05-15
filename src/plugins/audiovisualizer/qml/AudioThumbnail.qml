import QtQml
import QtQuick

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow

import DiffScope.VisualEditor

Item {
    id: d

    required property ClipViewModel clipViewModel
    required property bool visualVisible
    required property ProjectViewModelContext projectViewModelContext
    required property double viewportOffset
    required property double viewportLength

    AudioThumbnailWaveformThumbnail {
        anchors.fill: parent
        audioClip: d.projectViewModelContext?.getClipDocumentItemFromViewItem(d.clipViewModel) ?? null
        projectWindowInterface: d.projectViewModelContext?.windowHandle ?? null
        viewportOffset: d.viewportOffset
        viewportLength: d.viewportLength
        color: Theme.foregroundPrimaryColor
    }

}
