// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: d

    required property ProjectLifecycleAddOn addOn

    ActionItem {
        actionId: "org.diffscope.core.file.new"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.newProject())
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.file.newFromTemplate"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.newProjectFromTemplate())
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.file.open"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.openProject())
        }
    }
}
