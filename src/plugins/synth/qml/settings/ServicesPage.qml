// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

Item {
    id: page

    required property QtObject pageHandle
    required property var configurationModel
    property bool started: false
    readonly property TextMatcher matcher: TextMatcher {}
    readonly property var currentService: serviceList.currentItem?.serviceModel ?? null

    anchors.fill: parent

    function addService() {
        const index = page.configurationModel.addService()
        serviceList.currentIndex = index
        serviceList.positionViewAtIndex(index, ListView.Contain)
    }

    function removeCurrentService() {
        const index = serviceList.currentIndex
        if (!page.configurationModel.removeService(index))
            return
        serviceList.currentIndex = Math.min(index, serviceList.count - 1)
    }

    function moveCurrentService(offset) {
        const index = serviceList.currentIndex
        if (!page.configurationModel.moveService(index, offset))
            return
        serviceList.currentIndex = index + offset
        serviceList.positionViewAtIndex(serviceList.currentIndex, ListView.Contain)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: visible ? 8 : 0
            visible: text.length > 0
            text: page.pageHandle.errorMessage
            color: Theme.errorColor
            wrapMode: Text.Wrap
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12

            Frame {
                SplitView.preferredWidth: 220
                SplitView.minimumWidth: 220
                SplitView.maximumWidth: 400
                padding: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    ListView {
                        id: serviceList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        focus: true
                        model: page.configurationModel
                        currentIndex: -1

                        onCountChanged: {
                            if (count === 0)
                                currentIndex = -1
                            else if (currentIndex < 0)
                                currentIndex = 0
                            else if (currentIndex >= count)
                                currentIndex = count - 1
                        }

                        delegate: ItemDelegate {
                            id: serviceDelegate
                            required property var model
                            required property int index
                            property var serviceModel: model

                            width: ListView.view.width
                            padding: 4
                            leftPadding: 8
                            rightPadding: 8
                            highlighted: ListView.isCurrentItem
                            ThemedItem.flat: true
                            ThemedItem.controlType: highlighted ? SVS.CT_Accent : SVS.CT_Normal
                            background: ButtonRectangle {
                                control: serviceDelegate
                                checked: serviceDelegate.highlighted
                                flat: true
                            }
                            contentItem: RowLayout {
                                spacing: 8
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Label {
                                        Layout.fillWidth: true
                                        text: serviceDelegate.model.name.length > 0
                                              ? serviceDelegate.model.name
                                              : qsTr("Unnamed service")
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: serviceDelegate.model.baseUrl
                                        elide: Text.ElideMiddle
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                                    }
                                }
                                Label {
                                    visible: !serviceDelegate.model.enabled
                                    text: qsTr("Disabled")
                                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                                }
                            }
                            onClicked: serviceList.currentIndex = index
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: serviceList.count === 0
                            text: qsTr("No DSSP services configured")
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.borderColor
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 4
                        spacing: 2

                        ToolButton {
                            text: qsTr("Add Service")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/add"
                            focusPolicy: Qt.StrongFocus
                            Accessible.name: text
                            ToolTip.visible: hovered
                            ToolTip.text: text
                            onClicked: page.addService()
                        }
                        ToolButton {
                            text: qsTr("Delete")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/delete"
                            focusPolicy: Qt.StrongFocus
                            Accessible.name: text
                            ToolTip.visible: hovered
                            ToolTip.text: text
                            enabled: serviceList.currentIndex >= 0
                            onClicked: page.removeCurrentService()
                        }
                        ToolButton {
                            text: qsTr("Move Up")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/arrow_up"
                            focusPolicy: Qt.StrongFocus
                            Accessible.name: text
                            ToolTip.visible: hovered
                            ToolTip.text: text
                            enabled: serviceList.currentIndex > 0
                            onClicked: page.moveCurrentService(-1)
                        }
                        ToolButton {
                            text: qsTr("Move Down")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/arrow_down"
                            focusPolicy: Qt.StrongFocus
                            Accessible.name: text
                            ToolTip.visible: hovered
                            ToolTip.text: text
                            enabled: serviceList.currentIndex >= 0
                                     && serviceList.currentIndex + 1 < serviceList.count
                            onClicked: page.moveCurrentService(1)
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }

            Item {
                SplitView.fillWidth: true
                SplitView.fillHeight: true

                ScrollView {
                    anchors.fill: parent
                    visible: Boolean(page.currentService)
                    contentWidth: availableWidth

                    ServiceConfigurationEditor {
                        width: parent.width
                        currentService: page.currentService
                        matcher: page.matcher
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: !page.currentService
                    text: qsTr("Select a service to edit.")
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                }
            }
        }
    }
}
