import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import SVSCraft.UIComponents
import SVSCraft.Extras

import QActionKit

import ChorusKit.AppCore

import DiffScope.UIShell
import DiffScope.CorePlugin

HomeWindow {
    id: homeWindow
    required property IHomeWindow windowHandle
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
    onNewFileRequested: () => {
        windowHandle.triggerAction("core.file.new")
    }
    navigationActionsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "core.homeNavigation"
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
            actionId: "core.homeTool"
            context: homeWindow.windowHandle.actionContext
            onObjectAdded: (index, object) => {
                homeWindow.toolActionsModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                homeWindow.toolActionsModel.remove(index)
            }
        }
    }
    macosMenusModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: homeWindow.isMacOS ? "core.homeMenu" : ""
            context: homeWindow.windowHandle.actionContext
            onObjectAdded: (index, object) => {
                homeWindow.macosMenusModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                homeWindow.macosMenusModel.remove(index)
            }
        }
    }

    Settings {
        settings: PluginDatabase.settings
        category: "DiffScope.CorePlugin.HomeWindow"
        property alias recentFilesIsListView: homeWindow.recentFilesIsListView
    }
}