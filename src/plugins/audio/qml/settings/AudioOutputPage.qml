import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false

    anchors.fill: parent
    contentWidth: availableWidth

    readonly property TextMatcher matcher: TextMatcher {}

    ColumnLayout {
        width: page.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    Label {
                        text: qsTr("Audio driver")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Audio device")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox {
                            Layout.fillWidth: true
                        }
                        Button {
                            text: qsTr("Test")
                        }
                        Button {
                            text: qsTr("Control Panel")
                        }
                    }
                    Label {
                        text: qsTr("Sample rate")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Buffer size")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Device gain")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Slider {
                            Layout.fillWidth: true
                        }
                        SpinBox {

                        }
                    }
                    Label {
                        text: qsTr("Device pan")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Slider {
                            Layout.fillWidth: true
                        }
                        SpinBox {

                        }
                    }
                }
            }
        }
    }
}