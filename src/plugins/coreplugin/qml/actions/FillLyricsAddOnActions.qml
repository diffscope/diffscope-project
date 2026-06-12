import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.DspxModel as DspxModel

ActionCollection {
    id: d

    required property QtObject addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null

    ActionItem {
        actionId: "org.diffscope.core.edit.fillLyrics"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.anyItemsSelected
                     && d.windowHandle.projectDocumentContext.document.selectionModel.selectionType === DspxModel.SelectionModel.ST_Note
            onTriggered: Qt.callLater(() => d.addOn.fillLyrics())
        }
    }
}
