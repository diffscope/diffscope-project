import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {

    ActionItem {
        actionId: "core.file.new"
        Action {
            onTriggered: (o) => {
                CoreInterface.newFile(o?.Window.window ?? null)
            }
        }
    }

    ActionItem {
        actionId: "core.file.newFromTemplate"
        Action {
            onTriggered: (o) => {
                CoreInterface.newFileFromTemplate("", o?.Window.window ?? null)
            }
        }
    }

    ActionItem {
        actionId: "core.file.open"
        Action {
            onTriggered: (o) => {
                CoreInterface.openFile("", o?.Window.window ?? null)
            }
        }
    }

    ActionItem {
        actionId: "core.documentations"
        Action {
            onTriggered: () => {
                CoreAchievementsModel.triggerAchievementCompleted(CoreAchievementsModel.Achievement_Help);
            }
        }
    }

    ActionItem {
        actionId: "core.settings"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => CoreInterface.execSettingsDialog("", w))
            }
        }
    }

    ActionItem {
        actionId: "core.plugins"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => CoreInterface.execPluginsDialog(w))
            }
        }
    }

    ActionItem {
        actionId: "core.showHomeWindow"
        Action {
            onTriggered: () => {
                CoreInterface.showHome()
            }
        }
    }

    ActionItem {
        actionId: "core.exit"
        Action {
            onTriggered: () => {
                CoreInterface.exitApplicationGracefully()
            }
        }
    }

    ActionItem {
        actionId: "core.aboutApp"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => CoreInterface.execAboutAppDialog(w))
            }
        }
    }

    ActionItem {
        actionId: "core.aboutQt"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => CoreInterface.execAboutQtDialog(w))
            }
        }
    }

    ActionItem {
        actionId: "core.runDspxInspector"
        Action {
            readonly property Component inspectorComponent: DspxInspectorDialog {
            }
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => {
                    let inspector = inspectorComponent.createObject()
                    inspector.pos = Qt.point(w.x + 0.5 * (w.width - inspector.width), w.y + 0.5 * (w.height - inspector.height))
                    inspector.exec()
                })
            }
        }
    }

}