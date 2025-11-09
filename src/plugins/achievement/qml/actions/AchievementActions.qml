import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.UIShell
import DiffScope.Achievement

ActionCollection {
    id: d

    required property AchievementAddOn addOn

    ActionItem {
        actionId: "org.diffscope.achievement.achievements"
        Action {
            onTriggered: () => {
                let window = d.addOn.window
                window.show()
                if (window.visibility === Window.Minimized) {
                    window.showNormal()
                }
                window.raise()
                window.requestActivate()
            }
        }
    }

}