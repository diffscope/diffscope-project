import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

Dialog {
    id: dialog

    property MusicTimeline timeline
    property double tempo
    property int position
    property bool doInsertNew

    readonly property double minTempoBpm: 10
    readonly property double maxTempoBpm: 1000

    title: qsTr("Edit Tempo")

    onAboutToShow: tempoSpinBox.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2
        Label {
            id: tempoLabel
            text: qsTr("Tempo")
        }
        SpinBox {
            id: tempoSpinBox
            Accessible.labelledBy: tempoLabel
            Accessible.name: tempoLabel.text
            Layout.fillWidth: true
            readonly property int decimals: 2
            from: dialog.minTempoBpm * Math.pow(10, decimals)
            to: dialog.maxTempoBpm * Math.pow(10, decimals)
            stepSize: 100
            value: Math.round(dialog.tempo * Math.pow(10, decimals))
            onValueModified: () => {
                tapDetectButton.resetSession()
                dialog.tempo = value / Math.pow(10, decimals)
            }
            textFromValue: function(value, locale) {
                return Number(value / Math.pow(10, decimals)).toLocaleString(locale, 'f', decimals)
            }
            valueFromText: function(text, locale) {
                return Math.round(Number.fromLocaleString(locale, text) * Math.pow(10, decimals))
            }
            validator: DoubleValidator {
                bottom: 10
                top: 1000
                decimals: 2
            }
        }
        Item {

        }
        Button {
            id: tapDetectButton
            text: qsTr("Tap to Detect Tempo")
            DescriptiveText.toolTip: qsTr("Press the button to the rhythm to detect to detect the tempo. Right-click to cancel.")

            property int tapCount: 0
            property double sessionStartMs: 0
            property double lastTapMs: 0
            readonly property double sessionTimeoutMs: 2 * (60000 / dialog.minTempoBpm)

            highlighted: tapCount > 0

            Timer {
                id: sessionResetTimer
                interval: tapDetectButton.sessionTimeoutMs
                repeat: false
                onTriggered: tapDetectButton.resetSession()
            }

            function resetSession() {
                tapDetectButton.tapCount = 0
                tapDetectButton.sessionStartMs = 0
                tapDetectButton.lastTapMs = 0
                sessionResetTimer.stop()
            }

            onPressed: {
                const now = Date.now()
                if (tapDetectButton.lastTapMs > 0 && (now - tapDetectButton.lastTapMs) > tapDetectButton.sessionTimeoutMs) {
                    tapDetectButton.resetSession()
                }

                if (tapDetectButton.tapCount === 0) {
                    tapDetectButton.sessionStartMs = now
                    tapDetectButton.tapCount = 1
                } else {
                    tapDetectButton.tapCount += 1
                    const totalDuration = now - tapDetectButton.sessionStartMs
                    if (totalDuration > 0 && tapDetectButton.tapCount >= 2) {
                        let bpm = (tapDetectButton.tapCount - 1) * 60000 / totalDuration
                        bpm = Math.min(dialog.maxTempoBpm, Math.max(dialog.minTempoBpm, bpm))
                        dialog.tempo = bpm
                    }
                }

                tapDetectButton.lastTapMs = now
                sessionResetTimer.restart()
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                onSingleTapped: tapDetectButton.resetSession()
            }
        }
        Label {
            id: positionLabel
            text: qsTr("Position")
        }
        MusicTimeSpinBox {
            id: positionSpinBox
            Accessible.labelledBy: positionLabel
            Accessible.name: positionLabel.text
            Layout.fillWidth: true
            timeline: dialog.timeline
            value: dialog.position
            onValueModified: dialog.position = value
        }
        RowLayout {
            Layout.columnSpan: 2
            RadioButton {
                text: qsTr("Modify existing one")
                checked: !dialog.doInsertNew
                onClicked: dialog.doInsertNew = false
            }
            RadioButton {
                text: qsTr("Insert new one")
                checked: dialog.doInsertNew
                onClicked: dialog.doInsertNew = true
            }
        }
    }
    standardButtons: DialogButtonBox.Ok
}