// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: root
    required property QtObject addOn

    function taskTypeText(type) {
        return [qsTr("Pronunciation"), qsTr("Phoneme"), qsTr("Duration"), qsTr("Parameter"), qsTr("Audio")][type] ?? qsTr("Unknown");
    }

    function taskStateText(state) {
        return [qsTr("Queued"), qsTr("Running"), qsTr("Succeeded"), qsTr("Failed"), qsTr("Canceled")][state] ?? qsTr("Unknown");
    }

    function taskStateIcon(state) {
        return ["clock", "play_circle", "checkmark_circle", "dismiss_circle", "subtract_circle"][state] ?? "question_circle";
    }

    function taskStateColor(state) {
        if (state === 1)
            return Theme.accentColor;
        if (state === 3)
            return Theme.errorColor;
        return Theme.foregroundSecondaryColor;
    }

    readonly property Component panelComponent: ActionDockingPane {
        id: pane
        property double now: Date.now()
        onVisibleChanged: {
            if (visible)
                now = Date.now();
        }

        Timer {
            interval: 1000
            repeat: true
            running: pane.visible
            onTriggered: pane.now = Date.now()
        }

        header: ToolBarContainer {
            anchors.fill: parent
            ToolBarContainerStretch {}
            ToolButton {
                text: qsTr("Clear Diagnostics")
                display: AbstractButton.IconOnly
                icon.source: "image://fluent-system-icons/delete"
                ToolTip.visible: hovered
                ToolTip.text: text
                onClicked: {
                    if (pane.MessageBox.question(qsTr("Clear Diagnostics"), qsTr("Delete all saved synthesis diagnostics?")) === SVS.Yes)
                        root.addOn.clearDiagnostics();
                }
            }
            ToolButton {
                text: root.addOn?.refreshing ? qsTr("Refreshing...") : qsTr("Refresh")
                display: AbstractButton.IconOnly
                icon.source: "image://fluent-system-icons/arrow_sync"
                onClicked: root.addOn.refreshAll()
            }
        }

        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 8

                Repeater {
                    id: serviceRepeater
                    model: root.addOn?.serviceModel ?? null

                    delegate: Frame {
                        id: serviceDelegate
                        required property var model
                        required property int index
                        property bool expanded: false
                        readonly property int taskCount: model.taskCount
                        readonly property bool hasTasks: taskCount > 0
                        readonly property color statusColor: model.healthStatus === 4 ? Theme.errorColor : model.healthStatus === 3 ? Theme.accentColor : model.healthStatus === 2 ? Theme.warningColor : Theme.foregroundSecondaryColor
                        readonly property string statusDetails: {
                            let lines = [];
                            if (model.lastHealthCheck)
                                lines.push(qsTr("Last checked: %1").arg(model.lastHealthCheck));
                            if (model.errorMessage)
                                lines.push(model.errorMessage);
                            return lines.join("\n");
                        }

                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                        Layout.topMargin: index === 0 ? 8 : 0
                        Layout.bottomMargin: index === serviceRepeater.count - 1 ? 8 : 0
                        padding: 0
                        clip: true
                        Accessible.role: Accessible.ListItem
                        Accessible.name: model.name

                        background: Rectangle {
                            color: Theme.backgroundSecondaryColor
                            border.width: 1
                            border.color: Theme.borderColor
                            radius: 4
                        }

                        contentItem: ColumnLayout {
                            spacing: 0

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.margins: 12
                                spacing: 6

                                RowLayout {
                                    spacing: 8

                                    IconLabel {
                                        Layout.preferredWidth: 24
                                        Layout.preferredHeight: 24
                                        icon.source: "image://fluent-system-icons/cloud?size=24&style=regular"
                                        icon.color: Theme.foregroundSecondaryColor
                                        icon.width: 24
                                        icon.height: 24
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: serviceDelegate.model.name
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }

                                    ToolButton {
                                        Layout.preferredWidth: 28
                                        Layout.preferredHeight: 28
                                        text: serviceDelegate.expanded ? qsTr("Hide Tasks") : qsTr("Show Tasks")
                                        display: AbstractButton.IconOnly
                                        icon.source: serviceDelegate.expanded ? "image://fluent-system-icons/chevron_up" : "image://fluent-system-icons/chevron_down"
                                        ToolTip.visible: hovered
                                        ToolTip.text: text
                                        onClicked: serviceDelegate.expanded = !serviceDelegate.expanded
                                    }
                                }

                                RowLayout {
                                    spacing: 6

                                    RowLayout {
                                        spacing: 4

                                        IconLabel {
                                            icon.source: `image://fluent-system-icons/${serviceDelegate.model.healthIcon}`
                                            icon.color: serviceDelegate.statusColor
                                        }

                                        Label {
                                            text: serviceDelegate.model.healthText
                                            color: serviceDelegate.statusColor
                                        }

                                        ToolTip.visible: healthStatusHoverHandler.hovered && serviceDelegate.statusDetails.length > 0
                                        ToolTip.text: serviceDelegate.statusDetails

                                        HoverHandler {
                                            id: healthStatusHoverHandler
                                        }
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: qsTr("%L1 running, %L2 queued").arg(serviceDelegate.model.runningTaskCount).arg(serviceDelegate.model.queuedTaskCount)
                                        elide: Text.ElideRight
                                        horizontalAlignment: Text.AlignRight
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                                    }
                                }

                                RowLayout {
                                    spacing: 6

                                    IconLabel {
                                        icon.source: "image://fluent-system-icons/link"
                                        icon.color: Theme.foregroundSecondaryColor
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: serviceDelegate.model.baseUrl
                                        elide: Text.ElideMiddle
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                                        ToolTip.visible: urlHoverHandler.hovered && truncated
                                        ToolTip.text: text

                                        HoverHandler {
                                            id: urlHoverHandler
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                visible: serviceDelegate.expanded && serviceDelegate.hasTasks
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: Theme.borderColor
                            }

                            ColumnLayout {
                                visible: serviceDelegate.expanded && serviceDelegate.hasTasks
                                Layout.fillWidth: true
                                Layout.leftMargin: 12
                                Layout.rightMargin: 12
                                Layout.topMargin: 8
                                Layout.bottomMargin: 12
                                spacing: 4

                                Repeater {
                                    model: serviceDelegate.expanded ? serviceDelegate.model.tasks : null

                                    delegate: Rectangle {
                                        id: taskDelegate
                                        required property var task

                                        Layout.fillWidth: true
                                        Layout.preferredHeight: taskLayout.implicitHeight + 8
                                        color: Theme.backgroundTertiaryColor
                                        radius: 3
                                        Accessible.role: Accessible.ListItem
                                        Accessible.name: "%1, %2".arg(root.taskTypeText(task.type)).arg(root.taskStateText(task.state))

                                        ToolTip.visible: taskHoverHandler.hovered && !diagnosticsButton.hovered && !removeButton.hovered && task.state === 3 && Boolean(task.errorMessage)
                                        ToolTip.text: task.errorMessage

                                        HoverHandler {
                                            id: taskHoverHandler
                                        }

                                        RowLayout {
                                            id: taskLayout
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 8
                                            spacing: 8

                                            IconLabel {
                                                icon.source: `image://fluent-system-icons/${root.taskStateIcon(taskDelegate.task.state)}`
                                                icon.color: root.taskStateColor(taskDelegate.task.state)
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: root.taskTypeText(taskDelegate.task.type)
                                                elide: Text.ElideRight
                                            }

                                            Label {
                                                text: root.taskStateText(taskDelegate.task.state)
                                                color: root.taskStateColor(taskDelegate.task.state)
                                            }

                                            Label {
                                                visible: taskDelegate.task.state === 1 && Boolean(taskDelegate.task.startedAt)
                                                text: qsTr("%L1 s").arg(Math.max(0, Math.floor((pane.now - taskDelegate.task.startedAt.getTime()) / 1000)))
                                                ThemedItem.foregroundLevel: SVS.FL_Secondary
                                            }

                                            ToolButton {
                                                id: terminateButton
                                                visible: taskDelegate.task.state === 0 || taskDelegate.task.state === 1
                                                text: qsTr("Terminate Task")
                                                display: AbstractButton.IconOnly
                                                icon.source: "image://fluent-system-icons/stop"
                                                icon.color: Theme.errorColor
                                                onClicked: root.addOn.cancelTask(taskDelegate.task)
                                            }

                                            ToolButton {
                                                id: diagnosticsButton
                                                visible: taskDelegate.task.state === 3 && Boolean(taskDelegate.task.diagnosticFilePath)
                                                text: qsTr("View Diagnostics")
                                                display: AbstractButton.IconOnly
                                                icon.source: "image://fluent-system-icons/document_search"
                                                onClicked: DesktopServices.reveal(taskDelegate.task.diagnosticFilePath)
                                            }

                                            ToolButton {
                                                id: removeButton
                                                visible: taskDelegate.task.state === 3
                                                text: qsTr("Remove Task")
                                                display: AbstractButton.IconOnly
                                                icon.source: "image://fluent-system-icons/dismiss"
                                                onClicked: root.addOn.removeFailedTask(taskDelegate.task)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    visible: serviceRepeater.count === 0
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: qsTr("No synthesis services configured. Add one in Settings > Synthesis > Services.")
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                }
            }
        }
    }
}
