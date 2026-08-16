// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.Audio

ActionCollection {
    id: d

    required property ExportAudioAddOn addOn

    ActionItem {
        actionId: "org.diffscope.audio.export.exportAudio"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.exportAudio())
        }
    }
}
