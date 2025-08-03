import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import QActionKit

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.CorePlugin

QtObject {
    id: d

    property Window window: null
    property ProjectWindowData projectWindowData: window?.windowData ?? null

    readonly property Component newFile: Action {
        onTriggered: ICore.newFile()
    }
    readonly property Component openFile: Action {

    }
    readonly property Component settings: Action {
        onTriggered: (o) => {
            let w = o.Window.window
            Qt.callLater(() => ICore.showSettingsDialog("", w))
        }
    }
    readonly property Component plugins: Action {
        onTriggered: (o) => {
            let w = o.Window.window
            Qt.callLater(() => ICore.showPluginsDialog(w))
        }
    }
    readonly property Component showHome: Action {
        onTriggered: () => {
            ICore.showHome()
        }
    }
    readonly property Component workspaceDefaultLayout: Action {
        onTriggered: () => {
            d.projectWindowData.workspaceManager.currentLayout = d.projectWindowData.workspaceManager.defaultLayout
        }
    }
    readonly property Component workspaceCustomLayouts: Menu {
        title: ActionInstantiator.text
        icon.source: ActionInstantiator.iconSource
        readonly property list<var> customLayouts:  d.projectWindowData.workspaceManager.customLayouts
        readonly property Component customLayoutAction: Action {
            required property var layout
            text: layout?.name ?? ""
            onTriggered: () => {
                d.projectWindowData.workspaceManager.currentLayout = layout
            }
        }
        onCustomLayoutsChanged: () => {
            while (count) {
                removeItem(itemAt(0))
            }
            if (customLayouts.length === 0) {
                let action = customLayoutAction.createObject(this, {
                    layout: undefined,
                    enabled: false,
                    text: qsTr("None")
                })
                addAction(action)
                return
            }
            for (let o of customLayouts) {
                let action = customLayoutAction.createObject(this, {layout: o})
                addAction(action)
            }
        }

    }
    readonly property Component workspaceSaveLayout: Action {
        onTriggered: () => {
            d.window.promptSaveLayout()
        }

    }
    readonly property Component workspaceManageLayouts: Action {

    }

    readonly property Component allPanels: Menu {
        title: ActionInstantiator.text
        icon.source: ActionInstantiator.iconSource
        readonly property list<QtObject> allPanes: d.window.allPanes
        readonly property Component panelAction: Action {
            required property QtObject dockingPane
            text: dockingPane?.title ?? ""
            icon: dockingPane?.icon ?? GlobalHelper.defaultIcon()
            onTriggered: () => {
                dockingPane.Docking.dockingView.togglePane(dockingPane)
            }
        }
        onAllPanesChanged: () => {
            while (count) {
                removeItem(itemAt(0))
            }
            if (allPanes.length === 0) {
                let action = panelAction.createObject(this, {
                    dockingPane: null,
                    enabled: false,
                    text: qsTr("None")
                })
                addAction(action)
                return
            }
            for (let o of allPanes) {
                let action = panelAction.createObject(this, {dockingPane: o})
                addAction(action)

            }
        }
    }

    component WorkspacePanelAction: Action {
        id: action
        required property int panelPosition
        readonly property QtObject currentPane: d.window.dockingPanes[panelPosition]
        enabled: currentPane !== null
        readonly property string baseText: [
            "",
            qsTr("%1 (Left Top)"),
            qsTr("%1 (Left Bottom)"),
            qsTr("%1 (Right Top)"),
            qsTr("%1 (Right Bottom)"),
            qsTr("%1 (Top Left)"),
            qsTr("%1 (Top Right)"),
            qsTr("%1 (Bottom Left)"),
            qsTr("%1 (Bottom Right)"),
        ][panelPosition]
        icon: GlobalHelper.defaultIcon()
        readonly property Binding binding: Binding {
            when: action.currentPane !== null
            action.text: action.baseText.replace("%1", action.currentPane.title)
            action.icon: action.currentPane.icon
        }
        onTriggered: () => {
            d.projectWindowData.activePanel = panelPosition
            d.projectWindowData.activeUndockedPane = null
            d.projectWindowData.window.requestActivate()
        }
    }

    readonly property Component workspacePanelLeftTop: WorkspacePanelAction {
        panelPosition: ProjectWindowData.LeftTop
    }
    readonly property Component workspacePanelLeftBottom: WorkspacePanelAction {
        panelPosition: ProjectWindowData.LeftBottom
    }
    readonly property Component workspacePanelRightTop: WorkspacePanelAction {
        panelPosition: ProjectWindowData.RightTop
    }
    readonly property Component workspacePanelRightBottom: WorkspacePanelAction {
        panelPosition: ProjectWindowData.RightBottom
    }
    readonly property Component workspacePanelTopLeft: WorkspacePanelAction {
        panelPosition: ProjectWindowData.TopLeft
    }
    readonly property Component workspacePanelTopRight: WorkspacePanelAction {
        panelPosition: ProjectWindowData.TopRight
    }
    readonly property Component workspacePanelBottomLeft: WorkspacePanelAction {
        panelPosition: ProjectWindowData.BottomLeft
    }
    readonly property Component workspacePanelBottomRight: WorkspacePanelAction {
        panelPosition: ProjectWindowData.BottomRight
    }

    readonly property Component floatingPanels: Menu {
        title: ActionInstantiator.text
        icon.source: ActionInstantiator.iconSource
        readonly property list<QtObject> floatingPanes: d.window.floatingPanes
        readonly property Component floatingPanelAction: Action {
            required property QtObject dockingPane
            text: dockingPane?.title ?? ""
            icon: dockingPane?.icon ?? GlobalHelper.defaultIcon()
            onTriggered: () => {
                let window = dockingPane.Window.window
                if (window.visibility === Window.Minimized) {
                    window.showNormal()
                }
                window.raise()
                window.requestActivate()
            }
        }
        onFloatingPanesChanged: () => {
            while (count) {
                removeItem(itemAt(0))
            }
            if (floatingPanes.length === 0) {
                let action = floatingPanelAction.createObject(this, {
                    dockingPane: null,
                    enabled: false,
                    text: qsTr("None")
                })
                addAction(action)
                return
            }
            for (let o of floatingPanes) {
                let action = floatingPanelAction.createObject(this, {dockingPane: o})
                addAction(action)

            }
        }
    }

    readonly property Component dockActionToSideBar: Action {

    }

    readonly property Component menuBarVisible: Action {
        checkable: true
        checked: d.window.menuBar.visible
        onTriggered: () => {
            d.projectWindowData.toggleVisibility(ProjectWindowData.MenuBar, checked, this)
        }
    }
    readonly property Component toolBarVisible: Action {
        checkable: true
        checked: d.window.toolBar.visible
        onTriggered: () => {
            d.projectWindowData.toggleVisibility(ProjectWindowData.ToolBar, checked)
        }
    }
    readonly property Component leftSideBarVisible: Action {
        checkable: true
        checked: d.window.leftDockingView.barSize !== 0
        onTriggered: () => {
            d.projectWindowData.toggleVisibility(ProjectWindowData.LeftSideBar, checked)
        }
    }
    readonly property Component rightSideBarVisible: Action {
        checkable: true
        checked: d.window.rightDockingView.barSize !== 0
        onTriggered: () => {
            d.projectWindowData.toggleVisibility(ProjectWindowData.RightSideBar, checked, this)
        }
    }
    readonly property Component topSideBarVisible: Action {
        checkable: true
        checked: d.window.topDockingView.barSize !== 0
        onTriggered: () => {
            d.projectWindowData.toggleVisibility(ProjectWindowData.TopSideBar, checked)
        }
    }
    readonly property Component bottomSideBarVisible: Action {
        checkable: true
        checked: d.window.bottomDockingView.barSize !== 0
        onTriggered: () => {
            d.projectWindowData.toggleVisibility(ProjectWindowData.BottomSideBar, checked)
        }
    }
    readonly property Component statusBarVisible: Action {
        checkable: true
        checked: d.window.statusBar.visible
        onTriggered: () => {
            d.projectWindowData.toggleVisibility(ProjectWindowData.StatusBar, checked)
        }
    }

    readonly property Component documentations: Action {

    }
    readonly property Component findActions: Action {

    }
    readonly property Component aboutApp: Action {
        onTriggered: (o) => {
            let w = o.Window.window
            Qt.callLater(() => ICore.showAboutAppDialog(w))
        }
    }
    readonly property Component aboutQt: Action {
        onTriggered: (o) => {
            let w = o.Window.window
            Qt.callLater(() => ICore.showAboutQtDialog(w))
        }
    }
}