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
    readonly property Component workspaceLayouts: Menu {
        title: ActionInstantiator.text
        icon.source: ActionInstantiator.iconSource
        Action {
            text: qsTr("Default")
        }
        Menu {
            title: qsTr("Custom Layouts")
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Manage Layouts...")
        }
    }

    component WorkspacePanelAction: Action {
        id: action
        required property int panelPosition
        readonly property QtObject currentPane: d.projectWindowData.dockingPanes[panelPosition]
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
        readonly property list<QtObject> floatingPanes: d.projectWindowData.floatingPanes
        readonly property Component floatingPanelAction: Action {
            required property QtObject dockingPane
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
                action.text = Qt.binding(() => o.title)
                action.icon = Qt.binding(() => o.icon)
                addAction(action)

            }
        }
    }

    readonly property Component dockActionToSideBar: Action {

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