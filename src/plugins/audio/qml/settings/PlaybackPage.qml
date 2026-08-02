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
    property bool started: false

    property int playbackBehavior
    property int playbackTogglingAction

    readonly property var playbackBehaviorModel: [
        {text: qsTr("Return to start position"), value: AudioPreference.PB_ReturnToStart},
        {text: qsTr("Keep at current position"), value: AudioPreference.PB_KeepAtCurrent},
        {text: qsTr("Keep at current position, but play from start position next time"), value: AudioPreference.PB_KeepAtCurrentButPlayFromStart},
    ]
    readonly property var playbackTogglingActionModel: [
        {text: qsTr("Play/Stop"), value: AudioPreference.PTA_PlayStop},
        {text: qsTr("Play/Pause"), value: AudioPreference.PTA_PlayPause},
    ]

    onPlaybackBehaviorChanged: if (started) pageHandle.markDirty()
    onPlaybackTogglingActionChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    contentWidth: availableWidth

    readonly property TextMatcher matcher: TextMatcher {}

    ColumnLayout {
        width: page.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32
            GroupBox {
                title: qsTr("Playback")
                TextMatcherItem on title {
                    matcher: page.matcher
                }
                Layout.fillWidth: true
                GridLayout {
                    columns: 2
                    anchors.fill: parent
                    Label {
                        Layout.columnSpan: 2
                        text: qsTr("When playback stops, make playhead")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    ComboBox {
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        textRole: "text"
                        valueRole: "value"
                        model: page.playbackBehaviorModel
                        currentIndex: page.playbackBehavior
                        onCurrentValueChanged: page.playbackBehavior = currentValue
                    }
                    Label {
                        text: qsTr("Playback toggling action")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        textRole: "text"
                        valueRole: "value"
                        model: page.playbackTogglingActionModel
                        currentIndex: page.playbackTogglingAction
                        onCurrentValueChanged: page.playbackTogglingAction = currentValue
                    }
                }
            }
            GroupBox {
                title: qsTr("Metronome")
                TextMatcherItem on title {
                    matcher: page.matcher
                }
                Layout.fillWidth: true
                GridLayout {
                    columns: 2
                    anchors.fill: parent
                    CheckBox {
                        Layout.columnSpan: 2
                        text: qsTr("Enable metronome")
                        checked: page.pageHandle.metronomeEnabled
                        onToggled: page.pageHandle.metronomeEnabled = checked
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    Label {
                        text: qsTr("Metronome gain (dB)")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Slider {
                            Layout.fillWidth: true
                            from: SVS.decibelToLinearValue(-96)
                            to: SVS.decibelToLinearValue(6)
                            value: SVS.decibelToLinearValue(SVS.gainToDecibels(page.pageHandle.metronomeGain))
                            onMoved: page.pageHandle.metronomeGain = SVS.decibelsToGain(SVS.linearValueToDecibel(value))
                            ThemedItem.onDoubleClickReset: moved()
                        }
                        SpinBox {
                            id: metronomeGainSpinBox
                            property int decimals: 1
                            property real realValue: value / decimalFactor
                            readonly property int decimalFactor: Math.pow(10, decimals)

                            function decimalToInt(decimal) {
                                return decimal * decimalFactor
                            }

                            validator: DoubleValidator {
                                bottom: Math.min(metronomeGainSpinBox.from, metronomeGainSpinBox.to)
                                top: Math.max(metronomeGainSpinBox.from, metronomeGainSpinBox.to)
                                decimals: metronomeGainSpinBox.decimals
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
                            value: decimalToInt(SVS.gainToDecibels(page.pageHandle.metronomeGain))
                            onValueModified: page.pageHandle.metronomeGain = SVS.decibelsToGain(realValue)
                        }
                    }
                    Label {
                        text: qsTr("Metronome pan (%)")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Slider {
                            Layout.fillWidth: true
                            from: -1
                            to: 1
                            value: page.pageHandle.metronomePan
                            onMoved: page.pageHandle.metronomePan = value
                            ThemedItem.onDoubleClickReset: moved()
                        }
                        SpinBox {
                            from: -100
                            to: 100
                            value: Math.round(page.pageHandle.metronomePan * 100)
                            onValueModified: page.pageHandle.metronomePan = value / 100
                        }
                    }
                }
            }
        }
    }
}
