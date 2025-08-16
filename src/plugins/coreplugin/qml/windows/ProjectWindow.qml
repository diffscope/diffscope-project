import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

import SVSCraft
import SVSCraft.UIComponents

import QActionKit

import DiffScope.UIShell
import DiffScope.CorePlugin

ProjectWindow {
    id: projectWindow

    required property IProjectWindow windowHandle
    frameless: ICore.behaviorPreference.uiBehavior & BehaviorPreference.UB_Frameless
    useSeparatedMenu: !(ICore.behaviorPreference.uiBehavior & BehaviorPreference.UB_MergeMenuAndTitleBar)

    signal beforeTerminated()

    onClosing: () => {
        beforeTerminated()
    }

    menusModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "core.mainMenu"
            context: projectWindow.windowHandle.actionContext
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
            context: projectWindow.windowHandle.actionContext
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
    statusButtonsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "core.mainStatusBar"
            context: projectWindow.windowHandle.actionContext
            separatorComponent: ToolBarContainerSeparator {
            }
            stretchComponent: ToolBarContainerStretch {
            }
            onObjectAdded: (index, object) => {
                projectWindow.statusButtonsModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                projectWindow.statusButtonsModel.remove(index)
            }
        }
    }

}

