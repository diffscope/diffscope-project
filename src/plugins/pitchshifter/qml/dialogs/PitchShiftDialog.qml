// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Dialog {
    id: dialog

    property double stretch: 1
    property double pitch: 0
    property int formantMode: 0
    property double formantShift: 0
    property bool linkChannels: true

    width: 420
    title: qsTr("Stretch and Shift Pitch")
    standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

    onAboutToShow: stretchSpinBox.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2
        columnSpacing: 12
        rowSpacing: 8

        Label {
            id: stretchLabel
            text: qsTr("Stretch")
        }
        RowLayout {
            Layout.fillWidth: true
            SpinBox {
                id: stretchSpinBox
                readonly property int decimalFactor: 100
                property int decimals: 2

                Accessible.labelledBy: stretchLabel
                Accessible.name: stretchLabel.text
                Layout.fillWidth: true
                from: 10 * decimalFactor
                to: 1000 * decimalFactor
                value: Math.round(dialog.stretch * 100 * decimalFactor)
                editable: true
                validator: DoubleValidator {
                    bottom: 10
                    top: 1000
                    decimals: stretchSpinBox.decimals
                    notation: DoubleValidator.StandardNotation
                }
                textFromValue: function(value, locale) {
                    return Number(value / decimalFactor).toLocaleString(locale, "f", decimals)
                }
                valueFromText: function(text, locale) {
                    return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
                }
                onValueModified: dialog.stretch = value / decimalFactor / 100
            }
            Label {
                text: qsTr("%")
            }
        }

        Label {
            id: pitchLabel
            text: qsTr("Pitch")
        }
        RowLayout {
            Layout.fillWidth: true
            SpinBox {
                id: pitchSpinBox
                readonly property int decimalFactor: 100
                property int decimals: 2

                Accessible.labelledBy: pitchLabel
                Accessible.name: pitchLabel.text
                Layout.fillWidth: true
                from: -24 * decimalFactor
                to: 24 * decimalFactor
                value: Math.round(dialog.pitch * decimalFactor)
                editable: true
                validator: DoubleValidator {
                    bottom: -24
                    top: 24
                    decimals: pitchSpinBox.decimals
                    notation: DoubleValidator.StandardNotation
                }
                textFromValue: function(value, locale) {
                    return Number(value / decimalFactor).toLocaleString(locale, "f", decimals)
                }
                valueFromText: function(text, locale) {
                    return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
                }
                onValueModified: dialog.pitch = value / decimalFactor
            }
            Label {
                text: qsTr("semitones")
            }
        }


        Label {
            id: formantModeLabel
            text: qsTr("Formant Mode")
        }
        ComboBox {
            id: formantModeComboBox
            Accessible.labelledBy: formantModeLabel
            Accessible.name: formantModeLabel.text
            Layout.fillWidth: true
            model: [qsTr("Shift with pitch"), qsTr("Preserve"), qsTr("Custom")]
            currentIndex: dialog.formantMode
            onActivated: (index) => dialog.formantMode = index
        }

        Label {
            id: formantShiftLabel
            enabled: formantModeComboBox.currentIndex === 2
            text: qsTr("Formant Shift")
        }
        RowLayout {
            Layout.fillWidth: true
            SpinBox {
                id: formantShiftSpinBox
                readonly property int decimalFactor: 100
                property int decimals: 2

                Accessible.labelledBy: formantShiftLabel
                Accessible.name: formantShiftLabel.text
                Layout.fillWidth: true
                enabled: formantModeComboBox.currentIndex === 2
                from: -24 * decimalFactor
                to: 24 * decimalFactor
                value: Math.round(dialog.formantShift * decimalFactor)
                editable: true
                validator: DoubleValidator {
                    bottom: -24
                    top: 24
                    decimals: formantShiftSpinBox.decimals
                    notation: DoubleValidator.StandardNotation
                }
                textFromValue: function (value, locale) {
                    return Number(value / decimalFactor).toLocaleString(locale, "f", decimals)
                }
                valueFromText: function (text, locale) {
                    return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
                }
                onValueModified: dialog.formantShift = value / decimalFactor
            }
            Label {
                text: qsTr("semitones")
            }
        }

        CheckBox {
            Layout.columnSpan: 2
            text: qsTr("Link Channels")
            checked: dialog.linkChannels
            onToggled: dialog.linkChannels = checked
        }
    }
}
