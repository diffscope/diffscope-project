import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.Maintenance

ActionCollection {
    id: d

    required property ViewJsonAddOn addOn

    ActionItem {
        actionId: "org.diffscope.maintenance.diagnosis.viewJson"
        Action {
            onTriggered: () => {
                Qt.callLater(() => d.addOn.generateJsonFileAndOpen())
            }
        }
    }
}
