import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import QActionKit

import DiffScope.UIShell
import DiffScope.CorePlugin

ProjectWindow {
    id: projectWindow
    required property ProjectWindowData windowData
    menusModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "core.mainMenu"
            context: projectWindow.windowData.actionContext
            onObjectAdded: (index, object) => {
                projectWindow.menusModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                projectWindow.menusModel.remove(index)
            }
        }
    }
    toolButtonsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "core.mainToolBar"
            context: projectWindow.windowData.actionContext
            separatorComponent: ToolBarContainerSeparator {
            }
            stretchComponent: ToolBarContainerStretch {
            }
            onObjectAdded: (index, object) => {
                projectWindow.toolButtonsModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                projectWindow.toolButtonsModel.remove(index)
            }
        }
    }
}