import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.UIShell
import DiffScope.CorePlugin

ActionCollection {

    ActionItem {
        actionId: "core.newFile"
        Action {
            onTriggered: ICore.newFile()
        }
    }

    ActionItem {
        actionId: "core.openFile"
        Action {

        }
    }

    ActionItem {
        actionId: "core.documentations"
        Action {

        }
    }

    ActionItem {
        actionId: "core.findActions"
        Action {

        }
    }

    ActionItem {
        actionId: "core.settings"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => ICore.execSettingsDialog("", w))
            }
        }
    }

    ActionItem {
        actionId: "core.plugins"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => ICore.execPluginsDialog(w))
            }
        }
    }

    ActionItem {
        actionId: "core.showHome"
        Action {
            onTriggered: () => {
                ICore.showHome()
            }
        }
    }

    ActionItem {
        actionId: "core.exit"
        Action {
            onTriggered: () => {
                ICore.exitApplicationGracefully()
            }
        }
    }

    ActionItem {
        actionId: "core.aboutApp"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => ICore.execAboutAppDialog(w))
            }
        }
    }

    ActionItem {
        actionId: "core.aboutQt"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => ICore.execAboutQtDialog(w))
            }
        }
    }

}