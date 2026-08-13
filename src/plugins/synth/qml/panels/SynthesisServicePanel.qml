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
    property double now: Date.now()
    property Timer elapsedTimer: Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.now = Date.now()
    }

    function taskTypeText(type) {
        return [qsTr("Pronunciation"), qsTr("Phoneme"), qsTr("Duration"), qsTr("Parameter"), qsTr("Audio")][type] ?? qsTr("Unknown");
    }

    readonly property Component panelComponent: ActionDockingPane {
        id: pane

        header: ToolBarContainer {
            anchors.fill: parent
            ToolBarContainerStretch {}
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
                spacing: 0

                Repeater {
                    id: serviceRepeater
                    model: root.addOn?.serviceModel ?? null

                    delegate: ColumnLayout {
                        id: serviceDelegate
                        required property var model
                        required property int index
                        property bool expanded: false

                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                        Layout.topMargin: index === 0 ? 8 : 0
                        Layout.bottomMargin: index === serviceRepeater.count - 1 ? 8 : 0
                        spacing: 4

                        Card {
                            id: serviceCard
                            Layout.fillWidth: true
                            atTop: serviceDelegate.index === 0
                            atBottom: serviceDelegate.index === serviceRepeater.count - 1
                            title: RowLayout {
                                spacing: 4
                                Label {
                                    text: serviceDelegate.model.name
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                            subtitle: ColumnLayout {
                                spacing: 1
                                Label {
                                    Layout.fillWidth: true
                                    text: serviceDelegate.model.baseUrl
                                    elide: Text.ElideMiddle
                                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                                }
                                Label {
                                    text: qsTr("%1 running, %2 queued").arg(serviceDelegate.model.runningTaskCount).arg(serviceDelegate.model.queuedTaskCount)
                                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                                }
                            }
                            image: Rectangle {
                                color: Theme.backgroundQuaternaryColor

                                IconLabel {
                                    anchors.centerIn: parent
                                    icon.source: "image://fluent-system-icons/cloud?size=32&style=regular"
                                    icon.color: Theme.foregroundSecondaryColor
                                    icon.width: 32
                                    icon.height: 32
                                }
                            }
                            toolBar: RowLayout {
                                id: healthStatus

                                spacing: 4
                                readonly property color indicatorColor: serviceDelegate.model.healthStatus === 4 ? Theme.errorColor : serviceDelegate.model.healthStatus === 3 ? Theme.accentColor : serviceDelegate.model.healthStatus === 2 ? Theme.warningColor : Theme.foregroundSecondaryColor

                                IconLabel {
                                    icon.source: `image://fluent-system-icons/${serviceDelegate.model.healthIcon}`
                                    icon.color: healthStatus.indicatorColor
                                }
                                Label {
                                    text: serviceDelegate.model.healthText
                                    color: healthStatus.indicatorColor
                                }
                                ToolButton {
                                    visible: serviceDelegate.model.tasks.length > 0
                                    text: serviceDelegate.expanded ? qsTr("Hide Tasks") : qsTr("Show Tasks")
                                    display: AbstractButton.IconOnly
                                    icon.source: serviceDelegate.expanded ? "image://fluent-system-icons/chevron_up" : "image://fluent-system-icons/chevron_down"
                                    onClicked: serviceDelegate.expanded = !serviceDelegate.expanded
                                }
                            }
                            ToolTip.visible: hoverHandler.hovered
                            ToolTip.text: {
                                let lines = [];
                                if (serviceDelegate.model.lastHealthCheck)
                                    lines.push(qsTr("Last checked: %1").arg(serviceDelegate.model.lastHealthCheck));
                                if (serviceDelegate.model.errorMessage)
                                    lines.push(serviceDelegate.model.errorMessage);
                                return lines.join("\n");
                            }
                            HoverHandler {
                                id: hoverHandler
                            }
                        }

                        ColumnLayout {
                            visible: serviceDelegate.expanded && serviceDelegate.model.tasks.length > 0
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            spacing: 2

                            Repeater {
                                model: serviceDelegate.model.tasks

                                delegate: RowLayout {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 8
                                    IconLabel {
                                        icon.source: modelData.state === 1 ? "image://fluent-system-icons/play_circle" : "image://fluent-system-icons/clock"
                                    }
                                    Label {
                                        text: root.taskTypeText(modelData.type)
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: modelData.state === 1 ? qsTr("Running") : qsTr("Queued")
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                                    }
                                    Label {
                                        visible: modelData.state === 1 && Boolean(modelData.startedAt)
                                        text: qsTr("%1 s").arg(Math.max(0, Math.floor((root.now - modelData.startedAt.getTime()) / 1000)))
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
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
