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

    Connections {
        target: projectWindow.windowData
        function onInitialized() {
            Qt.callLater(() => {
                const f = (dockingView, edge) => {
                    let o = windowData.createDockingViewContents(edge)
                    dockingView.contentData = o.objects
                    dockingView.preferredPanelSize = o.preferredPanelSize
                    for (let i of o.visibleIndices) {
                        dockingView.showPane(i)
                    }
                }
                f(projectWindow.leftDockingView, Qt.LeftEdge)
                f(projectWindow.rightDockingView, Qt.RightEdge)
                f(projectWindow.topDockingView, Qt.TopEdge)
                f(projectWindow.bottomDockingView, Qt.BottomEdge)
            })
        }
    }

    Component {
        id: dockingStretchComponent
        DockingStretch {
        }
    }

    leftDockingView.contentData: [dockingStretchComponent.createObject()]
    rightDockingView.contentData: [dockingStretchComponent.createObject()]
    topDockingView.contentData: [dockingStretchComponent.createObject()]
    bottomDockingView.contentData: [dockingStretchComponent.createObject()]
}