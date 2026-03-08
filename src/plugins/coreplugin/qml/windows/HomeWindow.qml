import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import SVSCraft.UIComponents
import SVSCraft.Extras

import QActionKit

import ChorusKit.AppCore

import DiffScope.UIShell
import DiffScope.Core

HomeWindow {
    id: homeWindow
    required property HomeWindowInterface windowHandle
    frameless: BehaviorPreference.uiBehavior & BehaviorPreference.UB_Frameless
    readonly property color lightBannerColor: "#dadada"
    readonly property color darkBannerColor: "#252525"
    banner: {
        let c = ColorUtils.selectHighestContrastColor(Theme.backgroundPrimaryColor, [lightBannerColor, darkBannerColor]);
        if (c.r < 0.5) {
            return "qrc:/diffscope/coreplugin/logos/BannerDark.png";
        } else {
            return "qrc:/diffscope/coreplugin/logos/BannerLight.png";
        }
    }

    WindowSystem.windowSystem: CoreInterface.windowSystem
    WindowSystem.id: "org.diffscope.core.homewindow"

    onNewFileRequested: () => {
        windowHandle.triggerAction("org.diffscope.core.file.new", homeWindow.contentItem)
    }
    panelsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "org.diffscope.core.homePanels"
            context: homeWindow.windowHandle.actionContext
            onObjectAdded: (index, object) => {
                homeWindow.panelsModel.insert(index, object)
                let stateData = settings.value("panelData")?.[index] ?? undefined
                if (stateData && stateData.id === object.ActionInstantiator.id) {
                    if (object.loadState)
                        object.loadState(stateData.state)
                }
            }
            onObjectRemoved: (index, object) => {
                homeWindow.panelsModel.remove(index)
            }
        }
    }
    navigationActionsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "org.diffscope.core.homeNavigation"
            context: homeWindow.windowHandle.actionContext
            onObjectAdded: (index, object) => {
                homeWindow.navigationActionsModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                homeWindow.navigationActionsModel.remove(index)
            }
        }
    }
    toolActionsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "org.diffscope.core.homeTool"
            context: homeWindow.windowHandle.actionContext
            onObjectAdded: (index, object) => {
                homeWindow.toolActionsModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                homeWindow.toolActionsModel.remove(index)
            }
        }
    }
    menusModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "org.diffscope.core.homeMenu"
            context: homeWindow.windowHandle.actionContext
            onObjectAdded: (index, object) => {
                homeWindow.menusModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                homeWindow.menusModel.remove(index)
            }
        }
    }

    Settings {
        id: settings
        settings: RuntimeInterface.settings
        category: "DiffScope.Core.HomeWindow"
        property alias recentFilesIsListView: homeWindow.recentFilesIsListView
    }

    onClosing: () => {
        let a = []
        for (let i = 0; i < homeWindow.panelsModel.count; i++) {
            let pane = homeWindow.panelsModel.get(i)
            let state = pane.saveState ? pane.saveState() : null
            a.push({
                id: pane.ActionInstantiator.id,
                state
            })
        }
        settings.setValue("panelData", a)
    }
}