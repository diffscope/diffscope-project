// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick.Controls

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Audio

ActionCollection {
    id: collection

    required property MidiRecordingAddOn addOn

    ActionItem {
        actionId: "org.diffscope.audio.playback.record"
        Action {
            enabled: (collection.addOn?.recording ?? false) || (collection.addOn?.canRecord ?? false)
            checkable: true
            checked: (collection.addOn?.recording ?? false) && !(collection.addOn?.paused ?? false)
            onTriggered: {
                // The checked state must follow active capture during deferred commands.
                checked = Qt.binding(() => (collection.addOn?.recording ?? false) && !(collection.addOn?.paused ?? false))
                Qt.callLater(() => collection.addOn.startRecording())
            }
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
