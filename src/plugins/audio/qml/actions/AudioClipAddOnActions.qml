// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.UIShell
import DiffScope.Audio
import DiffScope.Core
import DiffScope.DspxModel as DspxModel
import DiffScope.DspxModel.SelectionModel as DspxSelectionModel

ActionCollection {
    id: d

    required property AudioClipAddOn addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property QtObject transactionController: windowHandle?.projectDocumentContext.document?.transactionController ?? null
    readonly property QtObject selectionModel: windowHandle?.projectDocumentContext.document?.selectionModel ?? null
    readonly property QtObject selectedAudioClip: selectionModel?.selectionType === DspxSelectionModel.SelectionModel.ST_Clip
        && selectionModel.selectedCount === 1
        && selectionModel.currentItem?.kind === DspxModel.Clip.Audio ? selectionModel.currentItem : null
    readonly property QtObject audioContext: AudioQmlHelper.getAudioClipAudioContext(selectedAudioClip)

    ActionItem {
        actionId: "org.diffscope.audio.insert.insertAudioClip"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
            onTriggered: Qt.callLater(() => d.addOn.insertAudioClip())
        }
    }

    ActionItem {
        actionId: "org.diffscope.audio.edit.replaceAudioClip"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
                     && d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Clip
                     && d.windowHandle?.projectDocumentContext.document.selectionModel.currentItem?.kind === DspxModel.Clip.Audio
            onTriggered: Qt.callLater(() => d.addOn.replaceAudioClip(d.windowHandle.projectDocumentContext.document.selectionModel.currentItem))
        }
    }

    ActionItem {
        actionId: "org.diffscope.audio.edit.embedAudioClip"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
                     && Boolean(d.selectedAudioClip) && !d.selectedAudioClip.path.embeddedKey
                     && Boolean(d.audioContext?.realAudioPath) && d.audioContext.status !== AudioClipAudioContext.FileLoadFailed
            onTriggered: Qt.callLater(() => d.addOn.embedAudioClip(d.selectedAudioClip))
        }
    }

    ActionItem {
        actionId: "org.diffscope.audio.edit.relinkAudioClip"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
                     && Boolean(d.selectedAudioClip?.path.embeddedKey)
                     && d.audioContext?.status === AudioClipAudioContext.Ready
            onTriggered: Qt.callLater(() => d.addOn.relinkAudioClip(d.selectedAudioClip))
        }
    }
}
