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
                title: qsTr("Font")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Use custom font")
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        Label {
                            text: qsTr("Font family")
                        }
                        ComboBox {
                            Layout.fillWidth: true
                        }
                        Label {
                            text: qsTr("Size")
                        }
                        SpinBox {
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("User Interface")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        CheckBox {
                            text: qsTr("Enable frameless window")
                        }
                        Label {
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            text: qsTr("(Restart required)")
                        }
                    }
                    CheckBox {
                        Layout.leftMargin: 22
                        text: qsTr("Merge menu bar and title bar")
                    }
                    RowLayout {
                        CheckBox {
                            text: qsTr("Use native menu bar")
                        }
                        Label {
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            text: qsTr("(Restart required)")
                        }
                    }
                    CheckBox {
                        text: qsTr("Show full path in the title bar of project window")
                    }
                }
            }
            GroupBox {
                title: qsTr("Graphics")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        CheckBox {
                            text: qsTr("Enable hardware acceleration")
                        }
                        Label {
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            text: qsTr("(Restart required)")
                        }
                    }
                    RowLayout {
                        CheckBox {
                            text: qsTr("Enable antialiasing")
                        }
                        Label {
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            text: qsTr("(Restart required)")
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Animation")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Enable animation")
                    }
                    RowLayout {
                        Label {
                            text: qsTr("Animation speed ratio")
                        }
                        SpinBox {

                        }
                    }
                }
            }
        }
    }
}