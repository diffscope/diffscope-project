// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.UIShell
import DiffScope.Audio
import DiffScope.Core

ActionCollection {
    id: d

    required property ExportAudioAddOn addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property QtObject transactionController: windowHandle?.projectDocumentContext.document?.transactionController ?? null

    ActionItem {
        actionId: "org.diffscope.audio.export.exportAudio"
        Action {
            enabled: d.transactionController?.canAcquireReadLock === true
            onTriggered: Qt.callLater(() => d.addOn.exportAudio())
        }
    }
}
