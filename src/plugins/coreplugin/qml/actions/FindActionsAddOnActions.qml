import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

ActionCollection {
    id: d

    required property QtObject addOn

    ActionItem {
        actionId: "org.diffscope.core.findActions"
        Action {
            onTriggered: () => {
                Qt.callLater(() => {
                    CoreAchievementsModel.triggerAchievementCompleted(CoreAchievementsModel.Achievement_FindActions);
                    d.addOn.findActions()
                })
            }
        }
    }

}