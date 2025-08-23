import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

ActionCollection {
    id: d

    required property QtObject addOn

    ActionItem {
        actionId: "core.findActions"
        Action {
            onTriggered: () => {
                d.addOn.findActions()
            }
        }
    }

}