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
    readonly property TextMatcher matcher: TextMatcher {}
    readonly property var currentArchitecture: architectureList.currentItem?.architectureModel ?? null

    anchors.fill: parent

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
                        id: architectureList
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
                            id: architectureDelegate
                            required property var model
                            required property int index
                            property var architectureModel: model

                            width: ListView.view.width
                            padding: 4
                            leftPadding: 8
                            rightPadding: 8
                            highlighted: ListView.isCurrentItem
                            ThemedItem.flat: true
                            ThemedItem.controlType: highlighted ? SVS.CT_Accent : SVS.CT_Normal
                            background: ButtonRectangle {
                                control: architectureDelegate
                                checked: architectureDelegate.highlighted
                                flat: true
                            }
                            contentItem: Label {
                                text: architectureDelegate.model.architectureId.length > 0
                                      ? architectureDelegate.model.architectureId
                                      : qsTr("Unnamed architecture")
                                elide: Text.ElideRight
                            }
                            onClicked: architectureList.currentIndex = index
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: architectureList.count === 0
                            text: qsTr("No architecture configurations")
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
                            text: qsTr("Add Architecture")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/add"
                            onClicked: architectureList.currentIndex = page.configurationModel.addEntry()
                        }
                        ToolButton {
                            text: qsTr("Delete")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/delete"
                            enabled: architectureList.currentIndex >= 0
                            onClicked: {
                                const oldIndex = architectureList.currentIndex
                                if (page.configurationModel.removeEntry(oldIndex))
                                    architectureList.currentIndex = Math.min(oldIndex, architectureList.count - 1)
                            }
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
                    visible: Boolean(page.currentArchitecture)
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width

                        GroupBox {
                            Layout.fillWidth: true
                            Layout.margins: 12
                            title: qsTr("Architecture Configuration")
                            TextMatcherItem on title {
                                matcher: page.matcher
                            }

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8

                                Label {
                                    text: qsTr("Architecture ID")
                                    TextMatcherItem on text {
                                        matcher: page.matcher
                                    }
                                }
                                TextField {
                                    Layout.fillWidth: true
                                    text: page.currentArchitecture?.architectureId ?? ""
                                    placeholderText: qsTr("Architecture ID")
                                    onTextEdited: page.currentArchitecture.architectureId = text
                                }
                                Label {
                                    text: qsTr("Configuration")
                                    TextMatcherItem on text {
                                        matcher: page.matcher
                                    }
                                }
                                ScrollView {
                                    Layout.fillWidth: true
                                    implicitHeight: 240
                                    TextArea {
                                        text: page.currentArchitecture?.json ?? ""
                                        placeholderText: qsTr("Enter a JSON value")
                                        wrapMode: TextEdit.NoWrap
                                        onTextChanged: if (activeFocus)
                                            page.currentArchitecture.json = text
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: !page.currentArchitecture
                    text: qsTr("Select an architecture to edit it.")
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                }
            }
        }
    }
}
