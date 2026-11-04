// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: d

    required property InsertItemAddOn addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property QtObject transactionController: windowHandle?.projectDocumentContext.document?.transactionController ?? null
    readonly property InsertItemScenario insertItemScenario: InsertItemScenario {
        id: insertItemScenario
        window: windowHandle?.window ?? null
        projectTimeline: windowHandle?.projectTimeline ?? null
        document: windowHandle?.projectDocumentContext.document ?? null
    }

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
            onTriggered: Qt.callLater(() => insertItemScenario.insertTrack())
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.insert.insertLabel"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
            onTriggered: Qt.callLater(() => insertItemScenario.insertLabel())
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.insert.insertSingingClip"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
            onTriggered: Qt.callLater(() => insertItemScenario.insertSingingClip())
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.insert.insertNote"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
                     && Boolean(d.windowHandle?.projectDocumentContext.document.selectionModel.noteSelectionModel.noteSequenceWithSelectedItems)
            onTriggered: Qt.callLater(() => insertItemScenario.insertNote())
        }
    }
}
