import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: root
    required property QtObject addOn

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

                    delegate: Card {
                        id: serviceCard
                        required property var model
                        required property int index

                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                        Layout.topMargin: index === 0 ? 8 : 0
                        Layout.bottomMargin: index === serviceRepeater.count - 1 ? 8 : 0
                        atTop: index === 0
                        atBottom: index === serviceRepeater.count - 1
                        title: RowLayout {
                            spacing: 4
                            Label {
                                text: serviceCard.model.name
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                        subtitle: Label {
                            text: serviceCard.model.baseUrl
                            elide: Text.ElideMiddle
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
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
                            readonly property color indicatorColor: serviceCard.model.healthStatus === 4
                                                                        ? Theme.errorColor
                                                                        : serviceCard.model.healthStatus === 3
                                                                          ? Theme.accentColor
                                                                          : serviceCard.model.healthStatus === 2
                                                                            ? Theme.warningColor
                                                                            : Theme.foregroundSecondaryColor

                            IconLabel {
                                icon.source: `image://fluent-system-icons/${serviceCard.model.healthIcon}`
                                icon.color: healthStatus.indicatorColor
                                icon.width: 16
                                icon.height: 16
                            }
                            Label {
                                text: serviceCard.model.healthText
                                color: healthStatus.indicatorColor
                            }
                        }
                        ToolTip.visible: hoverHandler.hovered
                        ToolTip.text: {
                            let lines = []
                            if (serviceCard.model.lastHealthCheck)
                                lines.push(qsTr("Last checked: %1").arg(serviceCard.model.lastHealthCheck))
                            if (serviceCard.model.errorMessage)
                                lines.push(serviceCard.model.errorMessage)
                            return lines.join("\n")
                        }
                        HoverHandler { id: hoverHandler }
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
