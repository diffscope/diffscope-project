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
        actionId: "org.diffscope.maintenance.diagnosis.generateDiagnosticReport"
        Action {
            onTriggered: () => {
                Qt.callLater(() => d.addOn.generateDiagnosticReport())
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.maintenance.diagnosis.openLogsDirectory"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.Logs)
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.maintenance.diagnosis.openDataDirectory"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.Data)
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.maintenance.diagnosis.openPluginsDirectory"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.Plugins)
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.maintenance.getInvolved.reportIssue"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.Issue)
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.maintenance.getInvolved.contribute"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.Contribute)
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.maintenance.getInvolved.joinTheCommunity"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.Community)
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.maintenance.update.checkForUpdates"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.maintenance.update.viewReleaseLog"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.ReleaseLog)
            }
        }
    }

}