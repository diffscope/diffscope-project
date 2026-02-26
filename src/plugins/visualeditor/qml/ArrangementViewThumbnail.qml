import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Effects

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views

Item {
    id: d
    required property ClipViewModel clipViewModel
    required property bool visualVisible
    required property PianoRollPanelInterface pianoRollPanelInterface
    required property ProjectViewModelContext projectViewModelContext

    Item {
        id: noteThumbnailViewport
        width: (d.clipViewModel?.length ?? 0) * noteThumbnail.horizontalFactor
        height: Math.max(10, noteThumbnail.maxKey - noteThumbnail.minKey + 1)
        clip: true
        NoteThumbnail {
            id: noteThumbnail
            color: Theme.foregroundPrimaryColor
            noteSequenceViewModel: d.clipViewModel?.associatedNoteSequence ?? null
            x: (-d.clipViewModel?.clipStart ?? 0) * horizontalFactor
            y: -0.5 * (127 - maxKey + 128 - minKey) + 0.5 * noteThumbnailViewport.height
        }
        transform: [
            Scale {
                origin.x: 0
                origin.y: 0
                xScale: d.width / noteThumbnailViewport.width
                yScale: d.height / noteThumbnailViewport.height
            }
        ]
    }

    Item {
        id: pianoRollRangeIndicator
        visible: d.pianoRollPanelInterface?.editingClip === d.projectViewModelContext?.getClipDocumentItemFromViewItem(d.clipViewModel)
        Binding {
            when: pianoRollRangeIndicator.visible
            pianoRollRangeIndicator.x: {
                if (!d.clipViewModel || !d.pianoRollPanelInterface?.timeViewModel)
                    return 0
                return (d.pianoRollPanelInterface.timeViewModel.start - d.clipViewModel.position) * d.width / d.clipViewModel.length
            }
            pianoRollRangeIndicator.width: {
                if (!d.clipViewModel || !d.pianoRollPanelInterface?.pianoRollView || !d.pianoRollPanelInterface?.timeLayoutViewModel)
                    return 0
                return (d.pianoRollPanelInterface.pianoRollView.centerEditArea.width / d.pianoRollPanelInterface.timeLayoutViewModel.pixelDensity) * d.width / d.clipViewModel.length
            }
            pianoRollRangeIndicator.y: {
                if (!d.pianoRollPanelInterface?.clavierViewModel || noteThumbnail.maxKey === -1)
                    return 0
                return (128 + noteThumbnail.y - d.pianoRollPanelInterface.clavierViewModel.start) * d.height / noteThumbnailViewport.height
            }
            pianoRollRangeIndicator.height: {
                if (!d.pianoRollPanelInterface?.pianoRollView || !d.pianoRollPanelInterface?.clavierViewModel)
                    return 0
                if (noteThumbnail.maxKey === -1)
                    return d.height
                return (d.pianoRollPanelInterface.pianoRollView.centerEditArea.height / d.pianoRollPanelInterface.clavierViewModel.pixelDensity) * d.height / noteThumbnailViewport.height
            }
        }
        Rectangle {
            color: Qt.rgba(Theme.foregroundPrimaryColor.r, Theme.foregroundPrimaryColor.g, Theme.foregroundPrimaryColor.b, 0.25 * Theme.foregroundPrimaryColor.a)
            border.color: Theme.foregroundPrimaryColor
            anchors.fill: parent
        }
    }
}