import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: d

    required property QtObject addOn
    required property QtObject helper
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property Window window: windowHandle?.window ?? null

    ActionItem {
        actionId: "org.diffscope.core.workspace.workspaceLayout"
        Action {
            onTriggered: () => {
                d.addOn.showWorkspaceLayoutCommand()
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.defaultLayout"
        Action {
            onTriggered: () => {
                d.addOn.workspaceManager.currentLayout = d.addOn.workspaceManager.defaultLayout
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.customLayouts"
        Menu {
            readonly property list<var> customLayouts: d.addOn.workspaceManager.customLayouts
            readonly property Component customLayoutAction: Menu {
                id: menu
                required property var layout
                title: layout?.name ?? ""
                Action {
                    text: qsTr("&Apply")
                    onTriggered: () => {
                        d.addOn.workspaceManager.currentLayout = layout
                    }
                }
                Action {
                    text: qsTr("&Rename")
                    onTriggered: () => {
                        Qt.callLater(() => d.helper.promptSaveLayout(d.addOn.workspaceManager.currentLayout, layout.name))
                    }
                }
                Action {
                    text: qsTr("&Delete")
                    onTriggered: () => {
                        Qt.callLater(() => d.helper.promptDeleteLayout(layout.name))
                    }
                }
            }
            readonly property Component dummyAction: Action {
                enabled: false
                text: qsTr("None")
            }
            onCustomLayoutsChanged: () => {
                while (count) {
                    removeItem(itemAt(0))
                }
                if (customLayouts.length === 0) {
                    let action = dummyAction.createObject(this)
                    addAction(action)
                    return
                }
                for (let o of customLayouts) {
                    let action = customLayoutAction.createObject(this, {layout: o})
                    addMenu(action)
                }
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.saveLayout"
        Action {
            onTriggered: () => {
                d.helper.saveCurrentLayout()
                Qt.callLater(() => d.helper.promptSaveLayout(d.addOn.workspaceManager.currentLayout, ""))
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.allPanels"
        Menu {
            readonly property list<QtObject> allPanes: d.helper.allPanes
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
    }

    component WorkspacePanelAction: Action {
        id: action
        required property int panelPosition
        readonly property QtObject currentPane: d.helper.dockingPanes[panelPosition]
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
            action.text: action.baseText.arg(action.currentPane.title)
            action.icon: action.currentPane.icon
        }
        onTriggered: () => {
            d.helper.activePanel = panelPosition
            d.helper.activeUndockedPane = null
            d.window.requestActivate()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.panelLeftTop"
        WorkspacePanelAction {
            panelPosition: WorkspaceAddOnHelper.LeftTop
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.panelLeftBottom"
        WorkspacePanelAction {
            panelPosition: WorkspaceAddOnHelper.LeftBottom
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.panelRightTop"
        WorkspacePanelAction {
            panelPosition: WorkspaceAddOnHelper.RightTop
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.panelRightBottom"
        WorkspacePanelAction {
            panelPosition: WorkspaceAddOnHelper.RightBottom
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.panelTopLeft"
        WorkspacePanelAction {
            panelPosition: WorkspaceAddOnHelper.TopLeft
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.panelTopRight"
        WorkspacePanelAction {
            panelPosition: WorkspaceAddOnHelper.TopRight
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.panelBottomLeft"
        WorkspacePanelAction {
            panelPosition: WorkspaceAddOnHelper.BottomLeft
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.panelBottomRight"
        WorkspacePanelAction {
            panelPosition: WorkspaceAddOnHelper.BottomRight
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.floatingPanels"
        Menu {
            readonly property list<QtObject> floatingPanes: d.helper.floatingPanes
            readonly property Component floatingPanelAction: Action {
                required property QtObject dockingPane
                text: dockingPane?.title ?? ""
                icon: dockingPane?.icon ?? GlobalHelper.defaultIcon()
                onTriggered: () => {
                    let window = dockingPane.Window.window
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
    }

    ActionItem {
        actionId: "org.diffscope.core.dockActionToSideBar"
        Menu {
            readonly property list<var> panelEntries: d.addOn.panelEntries
            readonly property Component addActionActionComponent: Action {
                text: qsTr("Add Action...")
            }
            readonly property Component panelEntryActionComponent: Action {
                required property var panelEntry
                text: panelEntry.text
                icon.source: panelEntry.iconSource
                icon.color: panelEntry.iconColor.valid ? panelEntry.iconColor : !enabled ? Theme.controlDisabledColorChange.apply(Theme.foregroundPrimaryColor) : Theme.foregroundPrimaryColor
                enabled: !panelEntry.unique || !d.helper.allPanes.some(o => o.ActionInstantiator.id === panelEntry.id)
                onTriggered: () => {
                    let o = d.windowHandle.actionContext.action(panelEntry.id)?.createObject(null)
                    if (!o || !(o instanceof Action || o instanceof DockingPane)) {
                        MessageBox.critical(qsTr("Error"), qsTr('Failed to create panel "%1"').arg(panelEntry.text))
                        if (o)
                            o.destroy()
                        return
                    }
                    d.windowHandle.actionContext.attachActionInfo(panelEntry.id, o)
                    d.helper.promptAddAction(o)
                }
            }
            readonly property Component menuSeparatorComponent: MenuSeparator {}
            onPanelEntriesChanged: () => {
                while (count) {
                    removeItem(itemAt(0))
                }
                for (let entry of panelEntries) {
                    let action = panelEntryActionComponent.createObject(this, {panelEntry: entry})
                    addAction(action)
                }
                if (panelEntries.length !== 0) {
                    addItem(menuSeparatorComponent.createObject(null))
                }
                addAction(addActionActionComponent.createObject(this))
            }
        }
    }

}