import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import SVSCraft.Extras

import QActionKit

import ChorusKit.AppCore

import DiffScope.UIShell
import DiffScope.CorePlugin

HomeWindow {
    id: homeWindow
    required property IHomeWindow windowHandle
    frameless: BehaviorPreference.uiBehavior & BehaviorPreference.UB_Frameless
    onNewFileRequested: () => {
        let newFileAction = windowHandle.actionContext.action("core.newFile").createObject()
        newFileAction.trigger()
        newFileAction.destroy()
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
            actionId: Qt.platform.os === "osx" || Qt.platform.os === "macos" ? "core.homeMenu" : ""
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