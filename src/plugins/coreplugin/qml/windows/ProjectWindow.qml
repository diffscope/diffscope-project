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
    frameless: BehaviorPreference.uiBehavior & BehaviorPreference.UB_Frameless
    useSeparatedMenu: !(BehaviorPreference.uiBehavior & BehaviorPreference.UB_MergeMenuAndTitleBar)

    icon: "image://appicon/dspx"

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
    component ToolButtonsObjectModel: ObjectModel {
        id: model
        property string actionId
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: model.actionId
            context: projectWindow.windowHandle.actionContext
            separatorComponent: ToolBarContainerSeparator {
            }
            stretchComponent: Item {
                visible: false
            }
            onObjectAdded: (index, object) => {
                model.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                model.remove(index)
            }
        }
    }
    leftToolButtonsModel: ToolButtonsObjectModel {
        actionId: "core.mainToolBarLeft"
    }
    rightToolButtonsModel: ToolButtonsObjectModel {
        actionId: "core.mainToolBarRight"
    }
    middleToolButtonsModel: ToolButtonsObjectModel {
        actionId: "core.mainToolBarMiddle"
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

