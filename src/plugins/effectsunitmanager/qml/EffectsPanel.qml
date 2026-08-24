// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: root

    required property QtObject addOn

    readonly property Component panelComponent: ActionDockingPane {
        id: pane

        MenuAction {
            id: addAction
            menu: Menu {
                id: addMenu
                title: qsTr("Add Effect")
                icon.source: "image://fluent-system-icons/add_circle"
                Instantiator {
                    model: root.addOn?.availableEffects ?? []
                    delegate: MenuItem {
                        required property var modelData
                        text: modelData.name
                        onTriggered: root.addOn.addEffect(modelData.id)
                    }
                    onObjectAdded: (index, object) => addMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => addMenu.removeItem(object)
                }
            }
        }

        header: ToolBarContainer {
            anchors.fill: parent
            ToolBarContainerStretch {}
            ToolButton {
                id: addButton
                text: qsTr("Add Effect")
                display: AbstractButton.IconOnly
                enabled: (root.addOn?.hasEffectsContext ?? false) && (root.addOn?.availableEffects.length ?? 0) > 0
                action: addAction
            }
        }

        TabBar {
            id: effectsTabBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 12
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            height: 30
            ThemedItem.flat: true
            onCurrentIndexChanged: {
                if (root.addOn)
                    root.addOn.activeTab = currentIndex
            }
            Component.onCompleted: currentIndex = root.addOn?.activeTab ?? 1

            Connections {
                target: root.addOn
                function onSelectionContextChanged() {
                    if (effectsTabBar.currentIndex !== root.addOn.activeTab)
                        effectsTabBar.currentIndex = root.addOn.activeTab
                }
            }

            TabButton {
                text: root.addOn?.trackTabText ?? qsTr("Track")
                visible: root.addOn?.trackTabVisible ?? false
                width: visible ? implicitWidth : 0
                ThemedItem.tabIndicator: SVS.TI_Bottom
            }
            TabButton {
                text: qsTr("Master")
                width: implicitWidth
                ThemedItem.tabIndicator: SVS.TI_Bottom
            }
        }

        Label {
            anchors.top: effectsTabBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.topMargin: 12
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignTop
            wrapMode: Text.Wrap
            text: root.addOn?.selectionMessage ?? ""
            visible: !root.addOn?.hasEffectsContext
            color: Theme.foregroundSecondaryColor
        }

        ColumnLayout {
            anchors.top: effectsTabBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.topMargin: 4
            spacing: 0
            visible: root.addOn?.hasEffectsContext ?? false

            Frame {
                Layout.fillWidth: true
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.topMargin: 8
                visible: root.addOn?.readingFilterConflict ?? false
                implicitHeight: visible ? conflictLabel.implicitHeight + topPadding + bottomPadding : 0
                Label {
                    id: conflictLabel
                    anchors.fill: parent
                    text: root.addOn?.readingFilterConflictMessage ?? ""
                    wrapMode: Text.Wrap
                    color: Theme.errorColor
                }
            }

            ListView {
                id: effectsList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 8
                topMargin: 8
                bottomMargin: 8
                model: root.addOn?.effectsModel ?? null

                Button {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 8
                    visible: effectsList.count === 0
                    action: addAction
                }

                delegate: Item {
                    id: effectDelegate

                    required property int index
                    required property var model

                    width: ListView.view.width
                    height: implicitHeight
                    implicitHeight: effectFrame.implicitHeight
                    Accessible.role: Accessible.ListItem
                    Accessible.name: model.name

                    Frame {
                        id: effectFrame
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        implicitHeight: effectLayout.implicitHeight + topPadding + bottomPadding
                        padding: 4

                        contentItem: ColumnLayout {
                            id: effectLayout
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                ToolButton {
                                    text: effectDelegate.model.expanded ? qsTr("Collapse Effect") : qsTr("Expand Effect")
                                    display: AbstractButton.IconOnly
                                    icon.source: "image://fluent-system-icons/chevron_right"
                                    icon.width: 16
                                    icon.height: 16
                                    contentItem.rotation: effectDelegate.model.expanded ? 90 : 0
                                    onClicked: root.addOn.setExpanded(effectDelegate.index, !effectDelegate.model.expanded)
                                }
                                ToolButton {
                                    text: effectDelegate.model.enabled ? qsTr("Disable Effect") : qsTr("Enable Effect")
                                    display: AbstractButton.IconOnly
                                    icon.source: "image://fluent-system-icons/power"
                                    checkable: true
                                    checked: effectDelegate.model.enabled
                                    onToggled: root.addOn.setEffectEnabled(effectDelegate.index, checked)
                                    ThemedItem.controlType: SVS.CT_Accent
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: effectDelegate.model.name
                                    elide: Text.ElideRight
                                }
                                ToolButton {
                                    text: qsTr("Move Effect Up")
                                    display: AbstractButton.IconOnly
                                    icon.source: "image://fluent-system-icons/arrow_up"
                                    enabled: effectDelegate.index > 0
                                    onClicked: root.addOn.moveEffect(effectDelegate.index, -1)
                                }
                                ToolButton {
                                    text: qsTr("Move Effect Down")
                                    display: AbstractButton.IconOnly
                                    icon.source: "image://fluent-system-icons/arrow_down"
                                    enabled: effectDelegate.index + 1 < effectsList.count
                                    onClicked: root.addOn.moveEffect(effectDelegate.index, 1)
                                }
                                ToolButton {
                                    text: qsTr("Delete Effect")
                                    display: AbstractButton.IconOnly
                                    icon.source: "image://fluent-system-icons/delete"
                                    onClicked: root.addOn.removeEffect(effectDelegate.index)
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                Layout.leftMargin: 8
                                Layout.rightMargin: 8
                                Layout.bottomMargin: 4
                                visible: effectDelegate.model.expanded && !effectDelegate.model.known
                                text: effectDelegate.model.error
                                wrapMode: Text.Wrap
                                color: Theme.errorColor
                            }

                            Item {
                                id: editorHost
                                Layout.fillWidth: true
                                Layout.leftMargin: 8
                                Layout.rightMargin: 8
                                Layout.bottomMargin: visible ? 4 : 0
                                visible: effectDelegate.model.expanded && effectDelegate.model.known
                                implicitHeight: visible && effectDelegate.model.editor
                                    ? effectDelegate.model.editor.implicitHeight
                                    : 0

                                Binding {
                                    target: effectDelegate.model.editor
                                    property: "parent"
                                    value: editorHost
                                    when: editorHost.visible && effectDelegate.model.editor
                                    restoreMode: Binding.RestoreBindingOrValue
                                }
                                Binding {
                                    target: effectDelegate.model.editor
                                    property: "width"
                                    value: editorHost.width
                                    when: editorHost.visible && effectDelegate.model.editor
                                    restoreMode: Binding.RestoreBindingOrValue
                                }
                                Binding {
                                    target: effectDelegate.model.editor
                                    property: "height"
                                    value: effectDelegate.model.editor?.implicitHeight ?? 0
                                    when: editorHost.visible && effectDelegate.model.editor
                                    restoreMode: Binding.RestoreBindingOrValue
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
