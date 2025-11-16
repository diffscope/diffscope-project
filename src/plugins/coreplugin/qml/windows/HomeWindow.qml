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
        settings: RuntimeInterface.settings
        category: "DiffScope.Core.HomeWindow"
        property alias recentFilesIsListView: homeWindow.recentFilesIsListView
    }
}