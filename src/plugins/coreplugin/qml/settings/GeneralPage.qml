import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

ScrollView {
    id: scrollView
    anchors.fill: parent
    ColumnLayout {
        width: scrollView.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32
            GroupBox {
                title: qsTr("Startup")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        spacing: 16
                        Label {
                            text: qsTr("When starting %1").replace("%1", Application.name)
                        }
                        RadioButton {
                            text: qsTr("Open the home window")
                        }
                        RadioButton {
                            text: qsTr("Create a new project")
                        }
                    }
                    CheckBox {
                        text: qsTr("Open previous projects on startup automatically")
                    }
                    CheckBox {
                        text: qsTr("Close the home window after opening a project")
                    }
                }
            }
            GroupBox {
                title: qsTr("Language")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Use system language")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Language")
                        }
                        ComboBox {
                            Layout.fillWidth: true
                        }
                        Label {
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            text: qsTr("(Restart required)")
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Notification")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Play sound alert when a notification bubble is sent")
                    }
                    RowLayout {
                        Label {
                            text: qsTr("Timeout for auto hiding notification bubbles")
                        }
                        SpinBox {

                        }
                        Label {
                            text: qsTr("milliseconds")
                        }
                    }
                    Button {
                        text: qsTr('Reset All "Do Not Show Again"')
                    }
                }
            }
            GroupBox {
                title: qsTr("Network Proxy")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RadioButton {
                        text: qsTr("No proxy")
                    }
                    RadioButton {
                        text: qsTr("Use system proxy")
                    }
                    RadioButton {
                        text: qsTr("Manually configure proxy")
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 22
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            Label {
                                text: qsTr("Type")
                            }
                            ComboBox {
                                Layout.fillWidth: true
                            }
                            Label {
                                text: qsTr("Hostname")
                            }
                            TextField {
                                Layout.fillWidth: true
                            }
                            Label {
                                text: qsTr("Port")
                            }
                            SpinBox {
                                Layout.fillWidth: true
                            }
                        }
                        CheckBox {
                            text: qsTr("Authentication")
                        }
                        GridLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 22
                            columns: 2
                            Label {
                                text: qsTr("Username")
                            }
                            TextField {
                                Layout.fillWidth: true
                            }
                            Label {
                                text: qsTr("Password")
                            }
                            TextField {
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Updates")
                Layout.fillWidth: true
                ColumnLayout {
                    Layout.fillWidth: true
                    CheckBox {
                        text: qsTr("Check for updates on startup")
                    }
                    RowLayout {
                        Label {
                            text: qsTr("Type of update to check for")
                        }
                        ComboBox {

                        }
                    }
                    Button {
                        text: qsTr("Check for Updates")
                    }
                }
            }
        }
    }
}