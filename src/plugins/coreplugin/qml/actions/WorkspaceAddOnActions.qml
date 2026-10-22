// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls

import QActionKit

import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: collection
    required property QtObject addOn
    required property QtObject helper
    readonly property QtObject session: addOn?.session ?? null
    readonly property bool usable: Boolean(addOn && session && !session.shuttingDown)

    component PanelAction: Action {
        id: panelAction
        required property var entry
        readonly property DockingPanel panel: entry?.panel ?? null
        text: entry.text
        icon.source: entry.iconSource
        checkable: true
        enabled: collection.usable && !collection.session.busy

        readonly property Binding iconColorBinding: Binding {
            target: panelAction
            property: "icon.color"
            value: panelAction.entry.iconColor
            when: panelAction.entry.iconColor.valid
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.workspaceLayout"

        Action {
            enabled: collection.usable
            onTriggered: collection.addOn.showWorkspaceLayoutCommand()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.defaultLayout"

        Action {
            enabled: collection.usable
            onTriggered: collection.addOn.workspaceManager.currentLayout = collection.addOn.workspaceManager.defaultLayout
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.saveLayout"

        Action {
            enabled: collection.usable && !collection.session.busy
            onTriggered: {
                collection.addOn.saveCurrentLayout()
                collection.helper.promptSaveLayout(collection.addOn.workspaceManager.currentLayout, "")
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.customLayouts"

        ActionMenu {
            id: layouts

            Instantiator {
                model: collection.addOn?.workspaceManager?.customLayouts ?? []
                delegate: Menu {
                    required property var modelData
                    title: modelData.name

                    MenuItem {
                        text: qsTr("Apply")
                        onTriggered: collection.addOn.workspaceManager.currentLayout = modelData
                    }

                    MenuItem {
                        text: qsTr("Rename")
                        onTriggered: collection.helper.promptSaveLayout(modelData, modelData.name)
                    }

                    MenuItem {
                        text: qsTr("Delete")
                        onTriggered: collection.helper.promptDeleteLayout(modelData.name)
                    }
                }
                onObjectAdded: (index, object) => layouts.insertMenu(index, object)
                onObjectRemoved: (index, object) => layouts.removeMenu(object)
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.allPanels"

        ActionMenu {
            id: allPanels

            Instantiator {
                model: collection.addOn?.panelEntries ?? []
                delegate: PanelAction {
                    id: allPanelAction
                    required property var modelData
                    entry: modelData
                    checked: Boolean(panel?.group)
                    onTriggered: {
                        const addPanel = checked
                        checked = Qt.binding(() => Boolean(allPanelAction.panel?.group))
                        if (addPanel)
                            collection.session.showPanel(entry.id)
                        else
                            collection.session.context.remove(panel)
                    }
                }
                onObjectAdded: (index, object) => allPanels.insertAction(index, object)
                onObjectRemoved: (index, object) => allPanels.removeAction(object)
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.activePanels"

        ActionMenu {
            id: activePanels

            Instantiator {
                model: activePanelRepeater.count === 0 ? 1 : 0
                delegate: MenuItem {
                    text: qsTr("No Open Panels")
                    enabled: false
                }
                onObjectAdded: (index, object) => activePanels.insertItem(0, object)
                onObjectRemoved: (index, object) => activePanels.removeItem(object)
            }

            Instantiator {
                id: activePanelRepeater
                model: (collection.addOn?.panelEntries ?? []).filter(entry => entry.panel?.group)
                delegate: PanelAction {
                    id: activePanelAction
                    required property var modelData
                    entry: modelData
                    checked: Boolean(panel) && collection.session?.context?.activePanel === panel
                    onTriggered: {
                        const activatePanel = checked
                        checked = Qt.binding(() => Boolean(activePanelAction.panel) && collection.session?.context?.activePanel === activePanelAction.panel)
                        if (activatePanel)
                            collection.session.showPanel(entry.id)
                        else if (panel?.group?.canHide(panel))
                            collection.session.context.hide(panel)
                    }
                }
                onObjectAdded: (index, object) => activePanels.insertAction(index, object)
                onObjectRemoved: (index, object) => activePanels.removeAction(object)
            }
        }
    }

    readonly property Instantiator showCommands: Instantiator {
        model: collection.addOn?.registeredPanelIds ?? []
        delegate: QtObject {
            id: showCommand
            required property string modelData

            readonly property Component actionComponent: Action {
                enabled: collection.usable
                onTriggered: collection.session.showPanel(showCommand.modelData)
            }
            Component.onCompleted: if (collection.usable)
                collection.addOn.windowHandle.actionContext.addAction(showCommand.modelData + ".show", actionComponent)
        }
    }
}
