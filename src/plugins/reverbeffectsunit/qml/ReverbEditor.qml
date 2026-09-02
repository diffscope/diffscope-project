// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Audio
import DiffScope.ReverbEffectsUnit

ColumnLayout {
    id: editor

    required property ReverbEffectsUnit effectsUnit

    implicitWidth: 360
    spacing: 8

    function linearPosition(value, minimum, maximum) {
        return (value - minimum) / (maximum - minimum)
    }

    function linearValue(position, minimum, maximum) {
        return minimum + position * (maximum - minimum)
    }

    function logarithmicPosition(value, minimum, maximum) {
        return Math.log(value / minimum) / Math.log(maximum / minimum)
    }

    function logarithmicValue(position, minimum, maximum) {
        return minimum * Math.pow(maximum / minimum, position)
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 4
        columnSpacing: 8

        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Size")
            unit: qsTr("ms")
            parameterValue: editor.effectsUnit?.sizeMilliseconds ?? 40
            minimum: 10
            maximum: 120
            defaultValue: 40
            decimals: 1
            positionFromValue: value => editor.logarithmicPosition(
                value, minimum, maximum)
            valueFromPosition: position => editor.logarithmicValue(
                position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewSizeMilliseconds(value)
            setValue: value => editor.effectsUnit.setSizeMilliseconds(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Decay")
            unit: qsTr("s")
            parameterValue: editor.effectsUnit?.decaySeconds ?? 0.8
            minimum: 0.02
            maximum: 6
            defaultValue: 0.8
            decimals: 2
            positionFromValue: value => editor.logarithmicPosition(
                value, minimum, maximum)
            valueFromPosition: position => editor.logarithmicValue(
                position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewDecaySeconds(value)
            setValue: value => editor.effectsUnit.setDecaySeconds(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Damping")
            unit: qsTr("%")
            parameterValue: editor.effectsUnit?.dampingPercent ?? 50
            minimum: 0
            maximum: 100
            defaultValue: 50
            decimals: 0
            positionFromValue: value => editor.linearPosition(
                value, minimum, maximum)
            valueFromPosition: position => editor.linearValue(
                position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewDampingPercent(value)
            setValue: value => editor.effectsUnit.setDampingPercent(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Pre-Delay")
            unit: qsTr("ms")
            parameterValue: editor.effectsUnit?.preDelayMilliseconds ?? 20
            minimum: 0
            maximum: 250
            defaultValue: 20
            decimals: 1
            positionFromValue: value => editor.linearPosition(
                value, minimum, maximum)
            valueFromPosition: position => editor.linearValue(
                position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewPreDelayMilliseconds(value)
            setValue: value => editor.effectsUnit.setPreDelayMilliseconds(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            text: qsTr("Dry")
        }
        Slider {
            id: mixSlider

            Layout.fillWidth: true
            Accessible.name: qsTr("Dry/Wet Mix")
            from: 0
            to: 100
            stepSize: 1
            value: editor.effectsUnit?.mixPercent ?? 25
            ThemedItem.doubleClickResetValue: 25
            onMoved: {
                editor.effectsUnit.previewMixPercent(value)
                if (!pressed)
                    editor.effectsUnit.commitPreview()
            }
            onPressedChanged: {
                if (!pressed)
                    editor.effectsUnit.commitPreview()
            }
            ThemedItem.onDoubleClickReset: {
                editor.effectsUnit.previewMixPercent(25)
                editor.effectsUnit.commitPreview()
            }
        }
        Label {
            text: qsTr("Wet")
        }
        DoubleSpinBox {
            Layout.minimumWidth: 64
            Accessible.name: qsTr("Dry/Wet Mix")
            editable: true
            decimals: 2
            from: 0
            to: 100
            value: editor.effectsUnit?.mixPercent ?? 25
            onValueModified: editor.effectsUnit.setMixPercent(value)
        }
        Label {
            text: qsTr("%")
            color: Theme.foregroundSecondaryColor
        }
    }
}
