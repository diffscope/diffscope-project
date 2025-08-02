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
    readonly property DockingPane activeDockingPaneObject: {
        if (windowData.activeUndockedPane)
            return windowData.activeUndockedPane
        return dockingPanes[windowData.activePanel] ?? null
    }
    readonly property list<QtObject> dockingPanes: [
        null,
        leftDockingView.firstItem,
        leftDockingView.lastItem,
        rightDockingView.firstItem,
        rightDockingView.lastItem,
        topDockingView.firstItem,
        topDockingView.lastItem,
        bottomDockingView.firstItem,
        bottomDockingView.lastItem
    ]
    readonly property list<QtObject> floatingPanes: [
        ...leftDockingView.undockedItems,
        ...rightDockingView.undockedItems,
        ...topDockingView.undockedItems,
        ...bottomDockingView.undockedItems
    ]

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

    leftDockingView.firstActive: !windowData.activeUndockedPane && windowData.activePanel === ProjectWindowData.LeftTop
    leftDockingView.onFirstActivated: () => {
        windowData.activeUndockedPane = null
        windowData.activePanel = ProjectWindowData.LeftTop
    }
    leftDockingView.lastActive: !windowData.activeUndockedPane && windowData.activePanel === ProjectWindowData.LeftBottom
    leftDockingView.onLastActivated: () => {
        windowData.activeUndockedPane = null
        windowData.activePanel = ProjectWindowData.LeftBottom
    }
    leftDockingView.activeUndockedPane: windowData.activeUndockedPane
    leftDockingView.onUndockedActivated: (pane) => windowData.activeUndockedPane = pane
    leftDockingView.onUndockedDeactivated: (pane) => {
        if (windowData.activeUndockedPane === pane)
            windowData.activeUndockedPane = null
    }


    rightDockingView.firstActive: !windowData.activeUndockedPane && windowData.activePanel === ProjectWindowData.RightTop
    rightDockingView.onFirstActivated: () => {
        windowData.activeUndockedPane = null
        windowData.activePanel = ProjectWindowData.RightTop
    }
    rightDockingView.lastActive: !windowData.activeUndockedPane && windowData.activePanel === ProjectWindowData.RightBottom
    rightDockingView.onLastActivated: () => {
        windowData.activeUndockedPane = null
        windowData.activePanel = ProjectWindowData.RightBottom
    }
    rightDockingView.activeUndockedPane: windowData.activeUndockedPane
    rightDockingView.onUndockedActivated: (pane) => windowData.activeUndockedPane = pane
    rightDockingView.onUndockedDeactivated: (pane) => {
        if (windowData.activeUndockedPane === pane)
            windowData.activeUndockedPane = null
    }


    topDockingView.firstActive: !windowData.activeUndockedPane && windowData.activePanel === ProjectWindowData.TopLeft
    topDockingView.onFirstActivated: () => {
        windowData.activeUndockedPane = null
        windowData.activePanel = ProjectWindowData.TopLeft
    }
    topDockingView.lastActive: !windowData.activeUndockedPane && windowData.activePanel === ProjectWindowData.TopRight
    topDockingView.onLastActivated: () => {
        windowData.activeUndockedPane = null
        windowData.activePanel = ProjectWindowData.TopRight
    }
    topDockingView.activeUndockedPane: windowData.activeUndockedPane
    topDockingView.onUndockedActivated: (pane) => windowData.activeUndockedPane = pane
    topDockingView.onUndockedDeactivated: (pane) => {
        if (windowData.activeUndockedPane === pane)
            windowData.activeUndockedPane = null
    }


    bottomDockingView.firstActive: !windowData.activeUndockedPane && windowData.activePanel === ProjectWindowData.BottomLeft
    bottomDockingView.onFirstActivated: () => {
        windowData.activeUndockedPane = null
        windowData.activePanel = ProjectWindowData.BottomLeft
    }
    bottomDockingView.lastActive: !windowData.activeUndockedPane && windowData.activePanel === ProjectWindowData.BottomRight
    bottomDockingView.onLastActivated: () => {
        windowData.activeUndockedPane = null
        windowData.activePanel = ProjectWindowData.BottomRight
    }
    bottomDockingView.activeUndockedPane: windowData.activeUndockedPane
    bottomDockingView.onUndockedActivated: (pane) => windowData.activeUndockedPane = pane
    bottomDockingView.onUndockedDeactivated: (pane) => {
        if (windowData.activeUndockedPane === pane)
            windowData.activeUndockedPane = null
    }

    onActiveDockingPaneObjectChanged: windowData.activeDockingPane = activeDockingPaneObject
    onDockingPanesChanged: windowData.dockingPanes = [...dockingPanes]
    onFloatingPanesChanged: () => windowData.floatingPanes = [...floatingPanes]
}