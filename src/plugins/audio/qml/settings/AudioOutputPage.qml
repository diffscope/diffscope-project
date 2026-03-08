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

    Component {
        id: troubleshootingDialogComponent
        AudioTroubleshootingDialog {
        }
    }

    Connections {
        target: page.helper
        function onDeviceError(message) {
            page.MessageBox.critical(qsTr("Audio device error"), message)
        }
    }

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
                        spacing: 8
                        Layout.fillWidth: true
                        Button {
                            text: qsTr("Troubleshoot...")
                            onClicked: () => {
                                let dialog = troubleshootingDialogComponent.createObject(null)
                                dialog.exec()
                                dialog.destroy()
                            }
                        }
                        Item {
                            Layout.fillWidth: true
                        }
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
                        text: qsTr("Device gain (dB)")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Slider {
                            Layout.fillWidth: true
                            from: SVS.decibelToLinearValue(-96)
                            to: SVS.decibelToLinearValue(6)
                            value: SVS.decibelToLinearValue(SVS.gainToDecibels(page.helper?.deviceGain ?? 1))
                            onMoved: page.helper.deviceGain = SVS.decibelsToGain(SVS.linearValueToDecibel(value))
                            ThemedItem.onDoubleClickReset: moved()
                        }
                        SpinBox {
                            id: gainSpinBox
                            property int decimals: 1
                            property real realValue: value / decimalFactor
                            readonly property int decimalFactor: Math.pow(10, decimals)

                            function decimalToInt(decimal) {
                                return decimal * decimalFactor
                            }

                            validator: DoubleValidator {
                                bottom: Math.min(gainSpinBox.from, gainSpinBox.to)
                                top:  Math.max(gainSpinBox.from, gainSpinBox.to)
                                decimals: gainSpinBox.decimals
                                notation: DoubleValidator.StandardNotation
                            }

                            textFromValue: function(value, locale) {
                                return Number(value / decimalFactor).toLocaleString(locale, 'f', decimals)
                            }

                            valueFromText: function(text, locale) {
                                return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
                            }

                            from: decimalToInt(-96)
                            to: decimalToInt(6)
                            value: decimalToInt(SVS.gainToDecibels(page.helper?.deviceGain ?? 1))

                            onValueModified: page.helper.deviceGain = SVS.decibelsToGain(realValue)
                        }
                    }
                    Label {
                        text: qsTr("Device pan (%)")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Slider {
                            Layout.fillWidth: true
                            from: -1
                            to: 1
                            value: page.helper?.devicePan ?? 0
                            onMoved: page.helper.devicePan = value
                            ThemedItem.onDoubleClickReset: moved()
                        }
                        SpinBox {
                            from: -100
                            to: 100
                            value: Math.round((page.helper?.devicePan ?? 0) * 100)
                            onValueModified: page.helper.devicePan = value / 100
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