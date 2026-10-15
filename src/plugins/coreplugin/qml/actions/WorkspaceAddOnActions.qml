// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls

import QActionKit

import SVSCraft.UIComponents

import DiffScope.Core

ActionCollection {
    id: collection
    required property QtObject addOn
    required property QtObject helper
    readonly property QtObject session: addOn?.session ?? null
    readonly property bool usable: Boolean(addOn && session && !session.shuttingDown)

    component PanelStateMenu: Menu {
        id: panelMenu
        required property var entry
        readonly property DockingPanel panel: entry?.panel ?? null
        readonly property bool hideAllowed: panel?.group?.canHide(panel) ?? false
        readonly property string stateText: panel?.shown ? qsTr("Shown") : panel?.group ? qsTr("Hidden") : qsTr("Unplaced")
        title: qsTr("%1 (%2)").arg(entry.text).arg(stateText)
        icon.source: entry.iconSource
        enabled: collection.usable

        Binding {
            target: panelMenu
            property: "icon.color"
            value: panelMenu.entry.iconColor
            when: panelMenu.entry.iconColor.valid
        }

        MenuItem {
            text: qsTr("Show and Activate")
            checkable: true
            autoExclusive: true
            checked: panelMenu.panel?.shown ?? false
            enabled: collection.usable && !collection.session.busy
            onTriggered: collection.session.showPanel(panelMenu.entry.id)
        }

        Instantiator {
            model: panelMenu.panel?.shown ? 1 : 0
            delegate: MenuItem {
                text: qsTr("Hide and Keep Tab")
                enabled: collection.usable && !collection.session.busy && panelMenu.hideAllowed
                DescriptiveText.statusTip: !panelMenu.hideAllowed ? qsTr("This panel group requires a visible panel.") : ""
                onTriggered: collection.session.context.hide(panelMenu.panel)
            }
            onObjectAdded: (index, object) => panelMenu.insertItem(1, object)
            onObjectRemoved: (index, object) => panelMenu.removeItem(object)
        }

        MenuItem {
            text: qsTr("Remove from Workspace")
            checkable: true
            autoExclusive: true
            checked: !panelMenu.panel?.group
            enabled: collection.usable && Boolean(panelMenu.panel?.group) && !collection.session.busy
            onTriggered: collection.session.context.remove(panelMenu.panel)
        }

        MenuSeparator {}

        MenuItem {
            text: qsTr("Float")
            enabled: collection.usable && Boolean(panelMenu.panel) && !collection.session.busy
            onTriggered: collection.session.context.requestFloat(panelMenu.panel)
        }

        MenuItem {
            text: qsTr("Reload Panel")
            enabled: collection.usable && Boolean(panelMenu.panel?.contentItem) && !collection.session.busy
            onTriggered: collection.session.retryPanel(panelMenu.entry.id)
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

        Menu {
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

        Menu {
            id: allPanels

            Instantiator {
                model: (collection.addOn?.panelEntries ?? []).filter(entry => entry.available)
                delegate: PanelStateMenu {
                    required property var modelData
                    entry: modelData
                }
                onObjectAdded: (index, object) => allPanels.insertMenu(index, object)
                onObjectRemoved: (index, object) => allPanels.removeMenu(object)
            }

            Menu {
                id: unavailablePanels
                title: qsTr("Unavailable Panels")
                enabled: unavailableRepeater.count > 0

                Instantiator {
                    id: unavailableRepeater
                    model: (collection.addOn?.panelEntries ?? []).filter(entry => !entry.available)
                    delegate: PanelStateMenu {
                        required property var modelData
                        entry: modelData
                    }
                    onObjectAdded: (index, object) => unavailablePanels.insertMenu(index, object)
                    onObjectRemoved: (index, object) => unavailablePanels.removeMenu(object)
                }
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.workspace.activePanels"

        Menu {
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
                model: (collection.addOn?.panelEntries ?? []).filter(entry => entry.panel?.shown)
                delegate: MenuItem {
                    required property var modelData
                    text: modelData.text
                    icon.source: modelData.iconSource
                    checkable: true
                    autoExclusive: true
                    checked: collection.session?.context?.activePanel === modelData.panel
                    enabled: collection.usable && !collection.session.busy
                    onTriggered: collection.session.context.activate(modelData.panel)
                }
                onObjectAdded: (index, object) => activePanels.insertItem(index, object)
                onObjectRemoved: (index, object) => activePanels.removeItem(object)
            }
        }
    }

    readonly property Instantiator showCommands: Instantiator {
        model: collection.addOn?.registeredPanelIds ?? []
        delegate: QtObject {
            required property string modelData

            readonly property Component actionComponent: Action {
                enabled: collection.usable && !collection.session.busy
                onTriggered: collection.session.showPanel(modelData)
            }
            Component.onCompleted: if (collection.usable)
                collection.addOn.windowHandle.actionContext.addAction(modelData + ".show", actionComponent)
        }
    }
}
