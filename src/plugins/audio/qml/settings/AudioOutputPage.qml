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
                            currentIndex: page.helper?.driverCurrentIndex ?? -1
                            onActivated: (index) => page.helper.driverCurrentIndex = index
                        }
                        Label {
                            text: qsTr("Audio device")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: page.helper?.deviceList ?? null
                            currentIndex: page.helper?.deviceCurrentIndex ?? -1
                            onActivated: (index) => page.helper.deviceCurrentIndex = index
                        }
                        Label {
                            text: qsTr("Sample rate")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: page.helper?.sampleRateList ?? null
                            currentIndex: page.helper?.sampleRateCurrentIndex ?? -1
                            onActivated: (index) => page.helper.sampleRateCurrentIndex = index
                        }
                        Label {
                            text: qsTr("Buffer size")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: page.helper?.bufferSizeList ?? null
                            currentIndex: page.helper?.bufferSizeCurrentIndex ?? -1
                            onActivated: (index) => page.helper.bufferSizeCurrentIndex = index
                        }
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        spacing: 8
                        Button {
                            text: qsTr("Test")
                            onClicked: page.helper.testDevice()
                        }
                        Button {
                            text: qsTr("Open Control Panel")
                            onClicked: page.helper.openControlPanel()
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