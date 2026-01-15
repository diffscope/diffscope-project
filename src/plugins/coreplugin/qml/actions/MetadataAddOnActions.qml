import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.Core

ActionCollection {
    id: d

    required property QtObject addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property EditMetadataScenario editMetadataScenario: EditMetadataScenario {
        id: metadataScenario
        window: windowHandle?.window ?? null
        document: windowHandle?.projectDocumentContext.document ?? null
    }

    ActionItem {
        actionId: "org.diffscope.core.file.metadata"
        Action {
            onTriggered: Qt.callLater(() => metadataScenario.editMetadata())
        }
    }
}
