// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.ChannelMapperEffectsUnit

ColumnLayout {
    id: editor

    required property ChannelMapperEffectsUnit effectsUnit

    spacing: 8

    GroupBox {
        id: leftGroupBox
        title: qsTr("Left")
        Layout.fillWidth: true
        GridLayout {
            anchors.fill: parent
            columns: 4
            columnSpacing: 8
            rowSpacing: 8

            Label {
                id: leftFromLeftLabel
                text: qsTr("Left")
            }
            Slider {
                id: leftFromLeftSlider
                Layout.fillWidth: true
                from: -100
                to: 100
                value: editor.effectsUnit?.leftLeftMixPercent ?? 0
                ThemedItem.doubleClickResetValue: 100
                Accessible.labelledBy: leftFromLeftLabel
                onMoved: {
                    editor.effectsUnit.previewLeftLeftMixPercent(value)
                    if (!pressed)
                        editor.effectsUnit.commitPreview()
                }
                onPressedChanged: {
                    if (!pressed)
                        editor.effectsUnit.commitPreview()
                }
                ThemedItem.onDoubleClickReset: {
                    editor.effectsUnit.previewLeftLeftMixPercent(100)
                    editor.effectsUnit.commitPreview()
                }
            }
            SpinBox {
                id: leftFromLeftSpinBox
                property int decimals: 1
                readonly property int decimalFactor: 10
                from: -100 * decimalFactor
                to: 100 * decimalFactor
                value: Math.round((editor.effectsUnit?.leftLeftMixPercent ?? 0) * decimalFactor)
                editable: true
                Accessible.labelledBy: leftFromLeftLabel
                validator: DoubleValidator {
                    bottom: -100
                    top: 100
                    decimals: leftFromLeftSpinBox.decimals
                    notation: DoubleValidator.StandardNotation
                }
                textFromValue: function(value, locale) {
                    return Number(value / decimalFactor).toLocaleString(locale, "f", decimals)
                }
                valueFromText: function(text, locale) {
                    return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
                }
                onValueModified: editor.effectsUnit.setLeftLeftMixPercent(value / decimalFactor)
            }
            Label {
                text: qsTr("%")
            }

            Label {
                id: leftFromRightLabel
                text: qsTr("Right")
            }
            Slider {
                id: leftFromRightSlider
                Layout.fillWidth: true
                from: -100
                to: 100
                value: editor.effectsUnit?.leftRightMixPercent ?? 0
                Accessible.labelledBy: leftFromRightLabel
                onMoved: {
                    editor.effectsUnit.previewLeftRightMixPercent(value)
                    if (!pressed)
                        editor.effectsUnit.commitPreview()
                }
                onPressedChanged: {
                    if (!pressed)
                        editor.effectsUnit.commitPreview()
                }
                ThemedItem.onDoubleClickReset: {
                    editor.effectsUnit.previewLeftRightMixPercent(0)
                    editor.effectsUnit.commitPreview()
                }
            }
            SpinBox {
                id: leftFromRightSpinBox
                property int decimals: 1
                readonly property int decimalFactor: 10
                from: -100 * decimalFactor
                to: 100 * decimalFactor
                value: Math.round((editor.effectsUnit?.leftRightMixPercent ?? 0) * decimalFactor)
                editable: true
                Accessible.labelledBy: leftFromRightLabel
                validator: DoubleValidator {
                    bottom: -100
                    top: 100
                    decimals: leftFromRightSpinBox.decimals
                    notation: DoubleValidator.StandardNotation
                }
                textFromValue: function(value, locale) {
                    return Number(value / decimalFactor).toLocaleString(locale, "f", decimals)
                }
                valueFromText: function(text, locale) {
                    return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
                }
                onValueModified: editor.effectsUnit.setLeftRightMixPercent(value / decimalFactor)
            }
            Label {
                text: qsTr("%")
            }
        }
    }

    GroupBox {
        id: rightGroupBox
        title: qsTr("Right")
        Layout.fillWidth: true
        GridLayout {
            anchors.fill: parent
            columns: 4
            columnSpacing: 8
            rowSpacing: 8

            Label {
                id: rightFromLeftLabel
                text: qsTr("Left")
            }
            Slider {
                id: rightFromLeftSlider
                Layout.fillWidth: true
                from: -100
                to: 100
                value: editor.effectsUnit?.rightLeftMixPercent ?? 0
                Accessible.labelledBy: rightFromLeftLabel
                onMoved: {
                    editor.effectsUnit.previewRightLeftMixPercent(value)
                    if (!pressed)
                        editor.effectsUnit.commitPreview()
                }
                onPressedChanged: {
                    if (!pressed)
                        editor.effectsUnit.commitPreview()
                }
                ThemedItem.onDoubleClickReset: {
                    editor.effectsUnit.previewRightLeftMixPercent(0)
                    editor.effectsUnit.commitPreview()
                }
            }
            SpinBox {
                id: rightFromLeftSpinBox
                property int decimals: 2
                readonly property int decimalFactor: 10
                from: -100 * decimalFactor
                to: 100 * decimalFactor
                value: Math.round((editor.effectsUnit?.rightLeftMixPercent ?? 0) * decimalFactor)
                editable: true
                Accessible.labelledBy: rightFromLeftLabel
                validator: DoubleValidator {
                    bottom: -100
                    top: 100
                    decimals: rightFromLeftSpinBox.decimals
                    notation: DoubleValidator.StandardNotation
                }
                textFromValue: function(value, locale) {
                    return Number(value / decimalFactor).toLocaleString(locale, "f", decimals)
                }
                valueFromText: function(text, locale) {
                    return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
                }
                onValueModified: editor.effectsUnit.setRightLeftMixPercent(value / decimalFactor)
            }
            Label {
                text: qsTr("%")
            }

            Label {
                id: rightFromRightLabel
                text: qsTr("Right")
            }
            Slider {
                id: rightFromRightSlider
                Layout.fillWidth: true
                from: -100
                to: 100
                value: editor.effectsUnit?.rightRightMixPercent ?? 0
                ThemedItem.doubleClickResetValue: 100
                Accessible.labelledBy: rightFromRightLabel
                onMoved: {
                    editor.effectsUnit.previewRightRightMixPercent(value)
                    if (!pressed)
                        editor.effectsUnit.commitPreview()
                }
                onPressedChanged: {
                    if (!pressed)
                        editor.effectsUnit.commitPreview()
                }
                ThemedItem.onDoubleClickReset: {
                    editor.effectsUnit.previewRightRightMixPercent(100)
                    editor.effectsUnit.commitPreview()
                }
            }
            SpinBox {
                id: rightFromRightSpinBox
                property int decimals: 2
                readonly property int decimalFactor: 10
                from: -100 * decimalFactor
                to: 100 * decimalFactor
                value: Math.round((editor.effectsUnit?.rightRightMixPercent ?? 0) * decimalFactor)
                editable: true
                Accessible.labelledBy: rightFromRightLabel
                validator: DoubleValidator {
                    bottom: -100
                    top: 100
                    decimals: rightFromRightSpinBox.decimals
                    notation: DoubleValidator.StandardNotation
                }
                textFromValue: function(value, locale) {
                    return Number(value / decimalFactor).toLocaleString(locale, "f", decimals)
                }
                valueFromText: function(text, locale) {
                    return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
                }
                onValueModified: editor.effectsUnit.setRightRightMixPercent(value / decimalFactor)
            }
            Label {
                text: qsTr("%")
            }
        }
    }
}
