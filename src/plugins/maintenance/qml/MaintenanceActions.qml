import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.UIShell
import DiffScope.Maintenance

ActionCollection {
    id: d

    required property MaintenanceAddOn addOn

    ActionItem {
        actionId: "maintenance.help.help"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "maintenance.diagnosis.generateDiagnosticInformation"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "maintenance.diagnosis.revealLogs"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "maintenance.diagnosis.openDataDirectory"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "maintenance.diagnosis.openPluginsDirectory"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "maintenance.getInvolved.reportIssue"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "maintenance.getInvolved.contribute"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "maintenance.getInvolved.joinTheCommunity"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "maintenance.update.checkForUpdates"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "maintenance.update.viewReleaseLog"
        Action {
            onTriggered: () => {

            }
        }
    }

}