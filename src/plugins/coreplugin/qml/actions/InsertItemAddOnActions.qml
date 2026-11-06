// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.UIShell
import DiffScope.Core
import DiffScope.DspxProject as DspxProject

ActionCollection {
    id: d

    required property InsertItemAddOn addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property QtObject transactionController: windowHandle?.projectDocumentContext.document?.transactionController ?? null
    readonly property DspxProject.InsertItemScenario insertItemScenario: windowHandle?.insertItemScenario ?? null

    ActionItem {
        actionId: "org.diffscope.core.insert.addTrack"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true
            onTriggered: insertItemScenario.addTrack()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.insert.insertTrack"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
            onTriggered: Qt.callLater(() => insertItemScenario.insertTrack(d.windowHandle.window))
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.insert.insertLabel"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
            onTriggered: Qt.callLater(() => insertItemScenario.insertLabel(d.windowHandle.window, d.windowHandle.projectTimeline))
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.insert.insertSingingClip"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
            onTriggered: Qt.callLater(() => insertItemScenario.insertSingingClip(d.windowHandle.window, d.windowHandle.projectTimeline))
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.insert.insertNote"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
                     && Boolean(d.windowHandle?.projectDocumentContext.document.selectionModel.noteSelectionModel.noteSequenceWithSelectedItems)
            onTriggered: Qt.callLater(() => insertItemScenario.insertNote(d.windowHandle.window, d.windowHandle.projectTimeline))
        }
    }
}
