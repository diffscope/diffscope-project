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
    property AudioOutputSettingsHelper helper: null

    ColumnLayout {
        width: page.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32
            GroupBox {
                title: qsTr("Device Parameters")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 16
                    anchors.fill: parent
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        Label {
                            text: qsTr("Audio driver")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: page.helper?.driverList ?? null
                        }
                        Label {
                            text: qsTr("Audio device")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: page.helper?.deviceList ?? null
                        }
                        Label {
                            text: qsTr("Sample rate")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: page.helper?.sampleRateList ?? null
                        }
                        Label {
                            text: qsTr("Buffer size")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: page.helper?.bufferSizeList ?? null
                        }
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        spacing: 8
                        Button {
                            text: qsTr("Test")
                        }
                        Button {
                            text: qsTr("Open Control Panel")
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Mixer")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    Label {
                        text: qsTr("Device gain")
                        TextMatcherItem on text { matcher: page.matcher }
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
                        TextMatcherItem on text { matcher: page.matcher }
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
            GroupBox {
                title: qsTr("Hot Plug")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    Label {
                        text: qsTr("When device change detected")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: [
                            qsTr("Always show notification"),
                            qsTr("Show notification only when the current device is removed"),
                            qsTr("Do not show notification")
                        ]
                    }
                }
            }
        }
    }
}