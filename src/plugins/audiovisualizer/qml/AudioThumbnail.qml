// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow

import DiffScope.DspxModel as DspxModel
import DiffScope.Audio
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

    sourceComponent: Item {
        id: d

        readonly property real ticksPerPixel: width > 0 ? (thumbnailLoader.clipViewModel?.length ?? 0) / width : 0
        readonly property real viewportOffset: thumbnailLoader.visibleClipRect.x * ticksPerPixel
        readonly property real viewportLength: thumbnailLoader.visibleClipRect.width * ticksPerPixel

        readonly property DspxModel.AudioClip audioClip: thumbnailLoader.audioClip
        readonly property QtObject projectAudioAddOn: AudioQmlHelper.getProjectAudioAddOn(thumbnailLoader.ViewLayer.windowHandle)
        property AudioClipAudioContext audioClipAudioContext: null

        function refreshAudioClipAudioContext() {
            audioClipAudioContext = AudioQmlHelper.getAudioClipAudioContext(audioClip)
        }

        onAudioClipChanged: refreshAudioClipAudioContext()
        onProjectAudioAddOnChanged: refreshAudioClipAudioContext()

        Component.onCompleted: refreshAudioClipAudioContext()

        Connections {
            target: d.projectAudioAddOn

            function onAudioClipAudioContextChanged(audioClip, context) {
                if (audioClip === d.audioClip) {
                    d.audioClipAudioContext = context
                }
            }
        }

        AudioThumbnailWaveformThumbnail {
            anchors.fill: parent
            audioClip: d.audioClip
            projectWindowInterface: thumbnailLoader.ViewLayer.windowHandle
            viewportOffset: d.viewportOffset
            viewportLength: d.viewportLength
            color: Theme.foregroundPrimaryColor
        }

        Rectangle {
            color: Theme.backgroundQuaternaryColor
            x: Math.min(Math.max(8, thumbnailLoader.visibleClipRect.x + 8), d.width - width - 8)
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            width: label.implicitWidth + 8
            height: label.implicitHeight + 8
            opacity: 0.9
            visible: {
                switch (d.audioClipAudioContext?.status) {
                    case AudioClipAudioContext.FileNotFound:
                    case AudioClipAudioContext.FileLoadFailed:
                    case AudioClipAudioContext.FileMoved:
                    case AudioClipAudioContext.FileContentChanged:
                        return true
                    default:
                        return false
                }
            }
            IconLabel {
                id: label
                anchors.centerIn: parent
                color: {
                    switch (d.audioClipAudioContext?.status) {
                        case AudioClipAudioContext.FileNotFound:
                        case AudioClipAudioContext.FileLoadFailed:
                            return Theme.errorColor
                        default:
                            return Theme.warningColor
                    }
                }
                icon.source: {
                    switch (d.audioClipAudioContext?.status) {
                        case AudioClipAudioContext.FileNotFound:
                        case AudioClipAudioContext.FileLoadFailed:
                            return "image://fluent-system-icons/error_circle"
                        default:
                            return "image://fluent-system-icons/warning"
                    }
                }
                icon.color: {
                    switch (d.audioClipAudioContext?.status) {
                        case AudioClipAudioContext.FileNotFound:
                        case AudioClipAudioContext.FileLoadFailed:
                            return Theme.errorColor
                        default:
                            return Theme.warningColor
                    }
                }
                text: {
                    switch (d.audioClipAudioContext?.status) {
                        case AudioClipAudioContext.FileNotFound:
                            return "Audio file not found"
                        case AudioClipAudioContext.FileLoadFailed:
                            return "Audio file failed to load"
                        case AudioClipAudioContext.FileMoved:
                            return "Audio file moved"
                        case AudioClipAudioContext.FileContentChanged:
                            return "Audio file content changed"
                    }
                    return ""
                }
            }
        }

    }
}
