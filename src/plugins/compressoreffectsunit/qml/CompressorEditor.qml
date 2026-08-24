// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow

import DiffScope.CompressorEffectsUnit
import DiffScope.Audio

ColumnLayout {
    id: editor

    required property CompressorEffectsUnit effectsUnit

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

    RowLayout {
        Layout.fillWidth: true
        Layout.minimumHeight: 180
        Layout.preferredHeight: 180
        spacing: 8

        Rectangle {
            id: graphFrame

            Layout.fillHeight: true
            Layout.fillWidth: true
            border.color: Theme.borderColor
            border.width: 1
            color: Theme.backgroundTertiaryColor
            radius: 0

            Canvas {
                id: graph

                readonly property color activeFillColor: Qt.rgba(
                    Theme.accentColor.r, Theme.accentColor.g,
                    Theme.accentColor.b, Theme.accentColor.a * 0.18)
                readonly property color guideColor: Theme.borderColor
                readonly property color inputColor: Theme.foregroundSecondaryColor
                readonly property color outputColor: Theme.accentColor
                readonly property double maximumDb: 0
                readonly property double minimumDb: -96

                anchors.fill: parent
                antialiasing: true

                function outputDb(inputDb) {
                    const threshold = editor.effectsUnit?.thresholdDb ?? -12
                    const ratio = editor.effectsUnit?.ratio ?? 4
                    return inputDb <= threshold
                        ? inputDb
                        : threshold + (inputDb - threshold) / ratio
                }

                function xFromDb(value) {
                    return (value - minimumDb) / (maximumDb - minimumDb) * width
                }

                function yFromDb(value) {
                    return height - (value - minimumDb) / (maximumDb - minimumDb) * height
                }

                onActiveFillColorChanged: requestPaint()
                onGuideColorChanged: requestPaint()
                onHeightChanged: requestPaint()
                onInputColorChanged: requestPaint()
                onOutputColorChanged: requestPaint()
                onWidthChanged: requestPaint()

                Connections {
                    target: editor.effectsUnit
                    function onLevelsChanged() {
                        graph.requestPaint()
                    }
                    function onRatioChanged() {
                        graph.requestPaint()
                    }
                    function onThresholdDbChanged() {
                        graph.requestPaint()
                    }
                }

                onPaint: {
                    const context = getContext("2d")
                    context.clearRect(0, 0, width, height)

                    const threshold = Math.min(Math.max(
                        editor.effectsUnit?.thresholdDb ?? -12, minimumDb), maximumDb)
                    const measuredInput = editor.effectsUnit?.inputLevelDb ?? minimumDb
                    const currentInput = measuredInput <= -96
                        ? minimumDb
                        : Math.min(Math.max(measuredInput, minimumDb), maximumDb)

                    context.strokeStyle = guideColor
                    context.lineWidth = 1
                    context.beginPath()
                    context.moveTo(xFromDb(threshold), height)
                    context.lineTo(xFromDb(threshold), yFromDb(threshold))
                    context.stroke()

                    context.fillStyle = activeFillColor
                    context.beginPath()
                    context.moveTo(0, height)
                    const fillSteps = 48
                    for (let index = 0; index <= fillSteps; ++index) {
                        const inputDb = minimumDb
                            + (currentInput - minimumDb) * index / fillSteps
                        context.lineTo(xFromDb(inputDb), yFromDb(outputDb(inputDb)))
                    }
                    context.lineTo(xFromDb(currentInput), height)
                    context.closePath()
                    context.fill()

                    context.strokeStyle = inputColor
                    context.lineWidth = 1
                    context.beginPath()
                    context.moveTo(0, height)
                    context.lineTo(width, 0)
                    context.stroke()

                    context.strokeStyle = outputColor
                    context.lineWidth = 2
                    context.beginPath()
                    const curveSteps = 64
                    for (let index = 0; index <= curveSteps; ++index) {
                        const inputDb = minimumDb
                            + (maximumDb - minimumDb) * index / curveSteps
                        const x = xFromDb(inputDb)
                        const y = yFromDb(outputDb(inputDb))
                        if (index === 0)
                            context.moveTo(x, y)
                        else
                            context.lineTo(x, y)
                    }
                    context.stroke()

                    context.fillStyle = outputColor
                    context.beginPath()
                    context.arc(xFromDb(currentInput), yFromDb(outputDb(currentInput)), 3, 0, Math.PI * 2)
                    context.fill()
                }
            }
        }

        Item {
            id: levelMeterArea

            readonly property double maximumDb: 0
            readonly property double minimumDb: -48
            readonly property double normalizedThreshold: Math.min(Math.max(
                ((editor.effectsUnit?.thresholdDb ?? -12) - minimumDb)
                    / (maximumDb - minimumDb), 0), 1)

            Layout.fillHeight: true
            implicitWidth: levelMeterLayout.implicitWidth

            RowLayout {
                id: levelMeterLayout

                anchors.fill: parent
                LayoutMirroring.enabled: false
                LayoutMirroring.childrenInherit: true
                spacing: 2

                CompressorLevelMeter {
                    Layout.fillHeight: true
                    Accessible.name: qsTr("Left Output")
                    value: editor.effectsUnit?.leftOutputLevelDb ?? -96
                }
                CompressorLevelMeter {
                    Layout.fillHeight: true
                    Accessible.name: qsTr("Left Gain Reduction")
                    from: 0
                    reversed: true
                    to: 48
                    value: editor.effectsUnit?.leftGainReductionDb ?? 0
                }
                CompressorLevelMeter {
                    Layout.fillHeight: true
                    Accessible.name: qsTr("Right Gain Reduction")
                    from: 0
                    reversed: true
                    to: 48
                    value: editor.effectsUnit?.rightGainReductionDb ?? 0
                }
                CompressorLevelMeter {
                    Layout.fillHeight: true
                    Accessible.name: qsTr("Right Output")
                    value: editor.effectsUnit?.rightOutputLevelDb ?? -96
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                color: Theme.foregroundPrimaryColor
                height: 1
                y: Math.round((1 - levelMeterArea.normalizedThreshold)
                              * (levelMeterArea.height - height))
                z: 1
            }
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 4
        columnSpacing: 8

        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Threshold")
            unit: qsTr("dB")
            parameterValue: editor.effectsUnit?.thresholdDb ?? -12
            minimum: -96
            maximum: 0
            defaultValue: -12
            decimals: 1
            positionFromValue: value => editor.linearPosition(value, minimum, maximum)
            valueFromPosition: position => editor.linearValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewThresholdDb(value)
            setValue: value => editor.effectsUnit.setThresholdDb(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Ratio")
            unit: qsTr(":1")
            parameterValue: editor.effectsUnit?.ratio ?? 4
            minimum: 1
            maximum: 100
            defaultValue: 4
            decimals: 1
            positionFromValue: value => editor.logarithmicPosition(value, minimum, maximum)
            valueFromPosition: position => editor.logarithmicValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewRatio(value)
            setValue: value => editor.effectsUnit.setRatio(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Attack")
            unit: qsTr("ms")
            parameterValue: editor.effectsUnit?.attackMilliseconds ?? 10
            minimum: 0.01
            maximum: 1000
            defaultValue: 10
            decimals: 2
            positionFromValue: value => editor.logarithmicPosition(value, minimum, maximum)
            valueFromPosition: position => editor.logarithmicValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewAttackMilliseconds(value)
            setValue: value => editor.effectsUnit.setAttackMilliseconds(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Release")
            unit: qsTr("ms")
            parameterValue: editor.effectsUnit?.releaseMilliseconds ?? 100
            minimum: 10
            maximum: 10000
            defaultValue: 100
            decimals: 1
            positionFromValue: value => editor.logarithmicPosition(value, minimum, maximum)
            valueFromPosition: position => editor.logarithmicValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewReleaseMilliseconds(value)
            setValue: value => editor.effectsUnit.setReleaseMilliseconds(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
    }
}
