import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.Core

ActionCollection {
    id: d

    required property InsertItemAddOn addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property InsertItemScenario insertItemScenario: InsertItemScenario {
        id: insertItemScenario
        window: windowHandle?.window ?? null
        projectTimeline: windowHandle?.projectTimeline ?? null
        document: windowHandle?.projectDocumentContext.document ?? null
    }

    ActionItem {
        actionId: "org.diffscope.core.insert.addTrack"
        Action {
            onTriggered: insertItemScenario.addTrack()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.insert.insertTrack"
        Action {
            onTriggered: Qt.callLater(() => insertItemScenario.insertTrack())
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.insert.insertLabel"
        Action {
            onTriggered: Qt.callLater(() => insertItemScenario.insertLabel())
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.insert.insertSingingClip"
        Action {
            onTriggered: Qt.callLater(() => insertItemScenario.insertSingingClip())
        }
    }
}