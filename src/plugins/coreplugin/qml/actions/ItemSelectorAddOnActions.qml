// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick.Controls

import QActionKit

ActionCollection {
    id: root

    required property QtObject addOn

    ActionItem {
        actionId: "org.diffscope.core.edit.itemSelector"
        Action {
            onTriggered: Qt.callLater(() => root.addOn.showItemSelector())
        }
    }
}
