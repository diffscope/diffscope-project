// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick.Controls

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Audio
import DiffScope.Core

ActionCollection {
    id: collection

    required property MidiRecordingAddOn addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property QtObject transactionController: windowHandle?.projectDocumentContext.document?.transactionController ?? null

    ActionItem {
        actionId: "org.diffscope.audio.playback.record"
        Action {
            enabled: (collection.addOn?.recording ?? false)
                     || (collection.transactionController?.canBeginTransaction === true && collection.addOn?.canRecord === true)
            onTriggered: Qt.callLater(() => collection.addOn.startRecording())
        }
    }

    ActionItem {
        actionId: "org.diffscope.audio.playback.recordButton"
        ToolButton {
            action: collection.addOn?.getRecordAction() ?? null
            display: AbstractButton.IconOnly
            highlighted: (collection.addOn?.recording ?? false) && !(collection.addOn?.paused ?? false)
            ThemedItem.controlType: SVS.CT_Accent
        }
    }
}
