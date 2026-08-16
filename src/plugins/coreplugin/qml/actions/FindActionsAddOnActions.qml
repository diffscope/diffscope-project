// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

ActionCollection {
    id: d

    required property QtObject addOn

    ActionItem {
        actionId: "org.diffscope.core.findActions"
        Action {
            onTriggered: () => {
                Qt.callLater(() => {
                    d.addOn.findActions()
                })
            }
        }
    }

}