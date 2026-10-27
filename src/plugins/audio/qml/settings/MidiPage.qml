// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Audio

ScrollView {
    id: page

    required property QtObject pageHandle
    property MidiInputSettingsHelper helper: null
    readonly property TextMatcher matcher: TextMatcher {}

    anchors.fill: parent
    contentWidth: availableWidth

    Connections {
        target: page.helper
        function onDeviceError(message) {
            page.MessageBox.critical(qsTr("MIDI Device Error"), message)
        }
    }

    ColumnLayout {
        width: page.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32

            GroupBox {
                title: qsTr("MIDI Input")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    Label {
                        id: deviceLabel
                        text: qsTr("Input device")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: page.helper?.deviceList ?? null
                        currentIndex: page.helper?.deviceCurrentIndex ?? -1
                        onActivated: (index) => page.helper.deviceCurrentIndex = index
                        Accessible.name: deviceLabel.text
                    }
                }
            }
            GroupBox {
                title: qsTr("MIDI Preview")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 16
                    CheckBox {
                        text: qsTr("Enable MIDI Preview")
                        checked: page.helper?.previewEnabled ?? false
                        onClicked: page.helper.previewEnabled = checked
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        enabled: page.helper?.previewEnabled ?? false
                        Label {
                            id: soundLabel
                            text: qsTr("Sound")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: page.helper?.previewTypeList ?? null
                            currentIndex: page.helper?.previewTypeCurrentIndex ?? -1
                            onActivated: (index) => page.helper.previewTypeCurrentIndex = index
                            Accessible.name: soundLabel.text
                        }
                        Label {
                            id: volumeLabel
                            text: qsTr("Preview volume (dB)")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Slider {
                                Layout.fillWidth: true
                                from: SVS.decibelToLinearValue(-96)
                                to: SVS.decibelToLinearValue(6)
                                value: SVS.decibelToLinearValue(SVS.gainToDecibels(page.helper?.previewGain ?? 1))
                                onMoved: page.helper.previewGain = SVS.decibelsToGain(SVS.linearValueToDecibel(value))
                                ThemedItem.onDoubleClickReset: moved()
                                Accessible.name: volumeLabel.text
                            }
                            DoubleSpinBox {
                                decimals: 1
                                from: -96
                                to: 6
                                value: SVS.gainToDecibels(page.helper?.previewGain ?? 1)
                                onValueModified: page.helper.previewGain = SVS.decibelsToGain(value)
                                Accessible.name: volumeLabel.text
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Item {
                            Layout.fillWidth: true
                        }
                        Button {
                            text: qsTr("Test") + "!".repeat(page.helper?.previewTestInputLength ?? 0)
                            enabled: page.helper?.previewEnabled ?? false
                            checkable: true
                            checked: page.helper?.previewTestPlaying ?? false
                            onClicked: page.helper.previewTestPlaying = checked
                            onActiveFocusChanged: if (!activeFocus) page.helper?.finishPreviewTestInput()
                            Keys.onPressed: (event) => {
                                if (!activeFocus || !page.helper?.previewTestPlaying)
                                    return
                                if (event.key === Qt.Key_Shift || event.key === Qt.Key_Control || event.key === Qt.Key_Alt || event.key === Qt.Key_Meta)
                                    return
                                const exclamation = event.text === "!" && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))
                                if (!event.isAutoRepeat)
                                    page.helper.inputPreviewTestText(exclamation ? event.text : "")
                                if (exclamation)
                                    event.accepted = true
                            }
                            DescriptiveText.toolTip: page.helper?.previewTestToolTip ?? ""
                            DescriptiveText.bindAccessibleDescription: true
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        Button {
                            text: qsTr("Interrupt All Notes")
                            onClicked: page.helper.interruptPreview()
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("MIDI Machine Control")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Enable MMC")
                        checked: page.helper?.mmcEnabled ?? false
                        onClicked: page.helper.mmcEnabled = checked
                        DescriptiveText.toolTip: qsTr("Allow the MIDI input device to play, pause, and stop the active project.")
                        DescriptiveText.bindAccessibleDescription: true
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                }
            }
        }
    }
}
