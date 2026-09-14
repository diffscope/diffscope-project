// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick.Controls

import QActionKit

import DiffScope.BatchProcess

ActionCollection {
    id: root

    required property JavaScriptConsoleAddOn addOn

    ActionItem {
        actionId: "org.diffscope.batchprocess.openJavaScriptDebugConsole"
        Action {
            onTriggered: Qt.callLater(() => root.addOn.showConsole())
        }
    }
}
