// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: d

    required property ProjectWindowInterface windowHandle

    ActionItem {
        actionId: "org.diffscope.core.file.save"
        Action {
            enabled: d.windowHandle.projectDocumentContext.fileLocker && d.windowHandle.projectDocumentContext.document.transactionController.canAcquireReadLock
            onTriggered: Qt.callLater(() => CoreInterface.projectLifecycleScenario.saveProject(d.windowHandle.projectDocumentContext, d.windowHandle.window))
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.file.saveAs"
        Action {
            enabled: d.windowHandle.projectDocumentContext.fileLocker && d.windowHandle.projectDocumentContext.document.transactionController.canAcquireReadLock
            onTriggered: Qt.callLater(() => CoreInterface.projectLifecycleScenario.saveProjectAs(d.windowHandle.projectDocumentContext, "", d.windowHandle.window))
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.file.saveCopyAs"
        Action {
            enabled: d.windowHandle.projectDocumentContext.document.transactionController.canAcquireReadLock
            onTriggered: Qt.callLater(() => CoreInterface.projectLifecycleScenario.saveProjectCopy(d.windowHandle.projectDocumentContext, "", d.windowHandle.window))
        }
    }
}
