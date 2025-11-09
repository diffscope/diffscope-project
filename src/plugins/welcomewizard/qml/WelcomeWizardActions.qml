import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.UIShell
import DiffScope.WelcomeWizard

ActionCollection {
    id: d

    required property WelcomeWizardAddOn addOn

    ActionItem {
        actionId: "org.diffscope.welcomewizard.welcomeWizard"
        Action {
            onTriggered: () => {
                Qt.callLater(() => d.addOn.execWelcomeWizard())
            }
        }
    }

}