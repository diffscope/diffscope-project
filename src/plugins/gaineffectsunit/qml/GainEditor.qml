// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.GainEffectsUnit

GridLayout {
    id: editor

    required property GainEffectsUnit effectsUnit

    columns: 4
    columnSpacing: 8
    rowSpacing: 8

    function sliderValueFromDb(value) {
        return SVS.decibelToLinearValue(value) - SVS.decibelToLinearValue(0)
    }

    function dbFromSliderValue(value) {
        return SVS.linearValueToDecibel(value + SVS.decibelToLinearValue(0))
    }

    Label {
        id: leftLabel
        text: qsTr("Left")
    }
    Slider {
        id: leftSlider
        Layout.fillWidth: true
        from: editor.sliderValueFromDb(-96)
        to: editor.sliderValueFromDb(6)
        value: editor.sliderValueFromDb(editor.effectsUnit?.leftGainDb ?? 0)
        Accessible.labelledBy: leftLabel
        onMoved: {
            editor.effectsUnit.previewLeftGainDb(editor.dbFromSliderValue(value))
            if (!pressed)
                editor.effectsUnit.commitPreview()
        }
        onPressedChanged: {
            if (!pressed)
                editor.effectsUnit.commitPreview()
        }
        ThemedItem.onDoubleClickReset: {
            editor.effectsUnit.previewLeftGainDb(0)
            editor.effectsUnit.commitPreview()
        }
    }
    SpinBox {
        id: leftSpinBox
        property int decimals: 2
        readonly property int decimalFactor: 10
        from: -96 * decimalFactor
        to: 6 * decimalFactor
        value: Math.round((editor.effectsUnit?.leftGainDb ?? 0) * decimalFactor)
        editable: true
        Accessible.labelledBy: leftLabel
        validator: DoubleValidator {
            bottom: -96
            top: 6
            decimals: leftSpinBox.decimals
            notation: DoubleValidator.StandardNotation
        }
        textFromValue: function(value, locale) {
            return Number(value / decimalFactor).toLocaleString(locale, "f", decimals)
        }
        valueFromText: function(text, locale) {
            return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
        }
        onValueModified: editor.effectsUnit.setLeftGainDb(value / decimalFactor)
    }
    Label {
        text: qsTr("dB")
    }

    Label {
        id: rightLabel
        text: qsTr("Right")
    }
    Slider {
        id: rightSlider
        Layout.fillWidth: true
        from: editor.sliderValueFromDb(-96)
        to: editor.sliderValueFromDb(6)
        value: editor.sliderValueFromDb(editor.effectsUnit?.rightGainDb ?? 0)
        Accessible.labelledBy: rightLabel
        onMoved: {
            editor.effectsUnit.previewRightGainDb(editor.dbFromSliderValue(value))
            if (!pressed)
                editor.effectsUnit.commitPreview()
        }
        onPressedChanged: {
            if (!pressed)
                editor.effectsUnit.commitPreview()
        }
        ThemedItem.onDoubleClickReset: {
            editor.effectsUnit.previewRightGainDb(0)
            editor.effectsUnit.commitPreview()
        }
    }
    SpinBox {
        id: rightSpinBox
        property int decimals: 2
        readonly property int decimalFactor: 10
        from: -96 * decimalFactor
        to: 6 * decimalFactor
        value: Math.round((editor.effectsUnit?.rightGainDb ?? 0) * decimalFactor)
        editable: true
        Accessible.labelledBy: rightLabel
        validator: DoubleValidator {
            bottom: -96
            top: 6
            decimals: rightSpinBox.decimals
            notation: DoubleValidator.StandardNotation
        }
        textFromValue: function(value, locale) {
            return Number(value / decimalFactor).toLocaleString(locale, "f", decimals)
        }
        valueFromText: function(text, locale) {
            return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
        }
        onValueModified: editor.effectsUnit.setRightGainDb(value / decimalFactor)
    }
    Label {
        text: qsTr("dB")
    }

    CheckBox {
        Layout.columnSpan: 4
        text: qsTr("Link channels")
        checked: editor.effectsUnit?.channelsLinked ?? true
        onToggled: editor.effectsUnit.setChannelsLinked(checked)
    }
}
