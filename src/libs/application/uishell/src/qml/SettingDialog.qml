import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

Window {
    id: dialog
    flags: Qt.Dialog
    width: 800
    height: 600
    title: qsTr("Settings")
    property double navigationWidth: 200
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundQuaternaryColor
        ColumnLayout {
            anchors.fill: parent
            spacing: 1
            SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Rectangle {
                    SplitView.fillHeight: true
                    SplitView.preferredWidth: dialog.navigationWidth
                    onWidthChanged: GlobalHelper.setProperty(dialog, "navigationWidth", width)
                    color: Theme.backgroundTertiaryColor
                    ColumnLayout {
                        anchors.fill: parent
                        TextField {
                            Layout.fillWidth: true
                            Layout.margins: 8
                            placeholderText: qsTr("Search")
                            ThemedItem.icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Search16Filled.svg"
                        }
                        TreeView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }
                }
                Rectangle {
                    SplitView.fillHeight: true
                    SplitView.fillWidth: true
                    color: Theme.backgroundPrimaryColor
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.margins: 12
                            spacing: 4
                            RowLayout {
                                spacing: 4
                                Repeater {
                                    id: breadcrumbRepeater
                                    model: ["Test 1", "Test 2", "Test 3"]
                                    delegate: RowLayout {
                                        id: breadcrumbItem
                                        required property string modelData
                                        required property int index
                                        spacing: 4
                                        Label {
                                            text: breadcrumbItem.modelData
                                            Layout.alignment: Qt.AlignVCenter
                                            font.weight: Font.DemiBold
                                        }
                                        ColorImage {
                                            height: 12
                                            width: 12
                                            color: Theme.foregroundPrimaryColor
                                            source: "qrc:/qt/qml/DiffScope/UIShell/assets/ChevronRight12Filled.svg"
                                            visible: index !== breadcrumbRepeater.count - 1
                                            Layout.alignment: Qt.AlignVCenter
                                        }
                                    }
                                }
                            }
                            Label {
                                text: "description"
                            }
                        }
                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }
                }
            }
            Rectangle {
                color: Theme.backgroundSecondaryColor
                Layout.fillWidth: true
                height: 60
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                    Button {
                        ThemedItem.controlType: SVS.CT_Accent
                        text: qsTr("OK")
                    }
                    Button {
                        text: qsTr("Cancel")
                    }
                    Button {
                        text: qsTr("Apply")
                    }
                }
            }
        }
    }
}