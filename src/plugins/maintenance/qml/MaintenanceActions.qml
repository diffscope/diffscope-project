// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

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
        actionId: "org.diffscope.maintenance.diagnosis.openConfigDirectory"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.Config)
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
        actionId: "org.diffscope.maintenance.diagnosis.openStateDirectory"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.State)
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.maintenance.diagnosis.openCacheDirectory"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.Cache)
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
        actionId: "org.diffscope.maintenance.diagnosis.openBuiltinPluginsDirectory"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.BuiltinPlugins)
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.maintenance.diagnosis.openSystemPluginsDirectory"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.SystemPlugins)
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.maintenance.diagnosis.openUserPluginsDirectory"
        Action {
            onTriggered: () => {
                d.addOn.reveal(MaintenanceAddOn.UserPlugins)
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