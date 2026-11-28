// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.AudioBridgeServer

ActionCollection {
    id: d

    required property DownloadBridgeAddOn addOn

    ActionItem {
        actionId: "org.diffscope.audiobridgeserver.downloadBridge"

        Action {
            onTriggered: {
                d.addOn.openDownloadPage()
            }
        }
    }
}
