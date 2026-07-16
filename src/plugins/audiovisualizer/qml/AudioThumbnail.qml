import QtQml
import QtQuick
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow

import DiffScope.DspxModel as DspxModel
import DiffScope.Audio
import DiffScope.VisualEditor

Item {
    id: d

    required property ClipViewModel clipViewModel
    required property bool visualVisible
    required property ProjectViewModelContext projectViewModelContext
    required property double viewportOffset
    required property double viewportLength

    readonly property DspxModel.AudioClip audioClip: projectViewModelContext?.getClipDocumentItemFromViewItem(clipViewModel) ?? null
    readonly property AudioClipAudioContext audioClipAudioContext: AudioQmlHelper.getAudioClipAudioContext(audioClip)

    AudioThumbnailWaveformThumbnail {
        anchors.fill: parent
        audioClip: d.audioClip
        projectWindowInterface: d.projectViewModelContext?.windowHandle ?? null
        viewportOffset: d.viewportOffset
        viewportLength: d.viewportLength
        color: Theme.foregroundPrimaryColor
    }

    Rectangle {
        color: Theme.backgroundQuaternaryColor
        x: Math.min(Math.max(8, d.viewportOffset * d.width / (d.clipViewModel?.length ?? 0) + 8), d.width - width - 8)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        width: label.implicitWidth + 8
        height: label.implicitHeight + 8
        opacity: 0.9
        visible: {
            switch (d.audioClipAudioContext?.status) {
                case AudioClipAudioContext.Unknown:
                case AudioClipAudioContext.Ready:
                    return false
                default:
                    return true
            }
        }
        IconLabel {
            id: label
            anchors.centerIn: parent
            icon.width: 16
            icon.height: 16
            spacing: 4
            font: Theme.font
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
