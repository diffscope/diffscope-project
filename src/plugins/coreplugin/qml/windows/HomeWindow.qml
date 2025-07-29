import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import QActionKit

import DiffScope.UIShell
import DiffScope.CorePlugin

HomeWindow {
    id: homeWindow
    required property HomeWindowData windowData
    onNewFileRequested: () => {
        let newFileAction = windowData.actionContext.action("core.newFile").createObject()
        newFileAction.trigger()
        newFileAction.destroy()
    }
    navigationActionsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "core.homeNavigation"
            context: homeWindow.windowData.actionContext
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
            context: homeWindow.windowData.actionContext
            onObjectAdded: (index, object) => {
                homeWindow.toolActionsModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                homeWindow.toolActionsModel.remove(index)
            }
        }
    }
}