// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

ColumnLayout {
    id: control

    required property var commitValue
    required property int decimals
    required property double defaultValue
    required property string label
    required property double maximum
    required property double minimum
    required property double parameterValue
    required property var positionFromValue
    required property var previewValue
    required property var setValue
    required property var valueFromPosition

    property string unit: ""

    readonly property int decimalFactor: Math.pow(10, decimals)

    spacing: 4

    Label {
        id: parameterLabel

        Layout.alignment: Qt.AlignHCenter
        text: control.label
        elide: Text.ElideRight
    }

    Dial {
        id: parameterDial

        Layout.alignment: Qt.AlignHCenter
        Layout.preferredHeight: 44
        Layout.preferredWidth: 44
        Accessible.labelledBy: parameterLabel
        from: 0
        to: 1
        stepSize: 0.001
        value: control.positionFromValue(control.parameterValue)
        ThemedItem.doubleClickResetValue: control.positionFromValue(
            control.defaultValue)
        onMoved: {
            control.previewValue(control.valueFromPosition(value))
            if (!pressed)
                control.commitValue()
        }
        onPressedChanged: {
            if (!pressed)
                control.commitValue()
        }
        ThemedItem.onDoubleClickReset: {
            control.previewValue(control.defaultValue)
            control.commitValue()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 2

        SpinBox {
            id: parameterSpinBox

            Layout.fillWidth: true
            Layout.minimumWidth: 52
            Accessible.labelledBy: parameterLabel
            editable: true
            from: Math.round(control.minimum * control.decimalFactor)
            to: Math.round(control.maximum * control.decimalFactor)
            value: Math.round(control.parameterValue * control.decimalFactor)
            validator: DoubleValidator {
                bottom: control.minimum
                top: control.maximum
                decimals: control.decimals
                notation: DoubleValidator.StandardNotation
            }
            textFromValue: function(value, locale) {
                return Number(value / control.decimalFactor).toLocaleString(
                    locale, "f", control.decimals)
            }
            valueFromText: function(text, locale) {
                return Math.round(Number.fromLocaleString(locale, text)
                                  * control.decimalFactor)
            }
            onValueModified: control.setValue(value / control.decimalFactor)
        }
        Label {
            visible: control.unit.length > 0
            text: control.unit
            color: Theme.foregroundSecondaryColor
        }
    }
}
