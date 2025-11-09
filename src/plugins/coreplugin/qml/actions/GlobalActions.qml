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
        actionId: "org.diffscope.core.file.new"
        Action {
            onTriggered: (o) => {
                CoreInterface.newFile(o?.Window.window ?? null)
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.file.newFromTemplate"
        Action {
            onTriggered: (o) => {
                CoreInterface.newFileFromTemplate("", o?.Window.window ?? null)
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.file.open"
        Action {
            onTriggered: (o) => {
                CoreInterface.openFile("", o?.Window.window ?? null)
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.documentations"
        Action {
            onTriggered: () => {
                CoreAchievementsModel.triggerAchievementCompleted(CoreAchievementsModel.Achievement_Help);
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.settings"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => CoreInterface.execSettingsDialog("", w))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.plugins"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => CoreInterface.execPluginsDialog(w))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.showHomeWindow"
        Action {
            onTriggered: () => {
                CoreInterface.showHome()
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.exit"
        Action {
            onTriggered: () => {
                CoreInterface.exitApplicationGracefully()
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.aboutApp"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => CoreInterface.execAboutAppDialog(w))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.aboutQt"
        Action {
            onTriggered: (o) => {
                let w = o.Window.window
                Qt.callLater(() => CoreInterface.execAboutQtDialog(w))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.runDspxInspector"
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