// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Audio
import DiffScope.DeEsserEffectsUnit

ColumnLayout {
    id: editor

    required property DeEsserEffectsUnit effectsUnit

    readonly property double minimumFrequencyHz: 100
    readonly property double maximumFrequencyHz: 12000
    readonly property double minimumBandwidthHz: 100
    readonly property double maximumBandwidthHz: 6000
    readonly property double leftFrequencyHz: Math.min(Math.max(
        (effectsUnit?.frequencyHz ?? 6000)
            - (effectsUnit?.bandwidthHz ?? 3000) * 0.5,
        minimumFrequencyHz), maximumFrequencyHz)
    readonly property double rightFrequencyHz: Math.min(Math.max(
        (effectsUnit?.frequencyHz ?? 6000)
            + (effectsUnit?.bandwidthHz ?? 3000) * 0.5,
        minimumFrequencyHz), maximumFrequencyHz)

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

    function frequencyFromX(x) {
        const position = Math.min(Math.max(
            x / Math.max(graphArea.width, 1), 0), 1)
        return logarithmicValue(
            position, minimumFrequencyHz, maximumFrequencyHz)
    }

    function xFromFrequency(frequencyHz) {
        return logarithmicPosition(Math.min(Math.max(
            frequencyHz, minimumFrequencyHz), maximumFrequencyHz),
            minimumFrequencyHz, maximumFrequencyHz) * graphArea.width
    }

    function previewLeftEdge(frequencyHz) {
        const minimum = Math.max(
            minimumFrequencyHz, rightFrequencyHz - maximumBandwidthHz)
        const maximum = rightFrequencyHz - minimumBandwidthHz
        effectsUnit.previewBandEdges(
            Math.min(Math.max(frequencyHz, minimum), maximum),
            rightFrequencyHz)
    }

    function previewRightEdge(frequencyHz) {
        const minimum = leftFrequencyHz + minimumBandwidthHz
        const maximum = Math.min(
            maximumFrequencyHz, leftFrequencyHz + maximumBandwidthHz)
        effectsUnit.previewBandEdges(
            leftFrequencyHz,
            Math.min(Math.max(frequencyHz, minimum), maximum))
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.minimumHeight: 120
        Layout.preferredHeight: 120
        spacing: 8

        Rectangle {
            id: graphFrame

            Layout.fillHeight: true
            Layout.fillWidth: true
            border.color: Theme.borderColor
            border.width: 1
            clip: true
            color: Theme.backgroundTertiaryColor
            radius: 0

            Item {
                id: graphArea

                anchors.fill: parent
                anchors.margins: 1
                clip: true
                Accessible.role: Accessible.Graphic
                Accessible.name: qsTr("De-esser band and output spectrum")

                DeEsserSpectrumGraph {
                    anchors.fill: parent
                    effectsUnit: editor.effectsUnit
                    spectrumColor: Qt.rgba(
                        Theme.accentColor.r, Theme.accentColor.g,
                        Theme.accentColor.b, Theme.accentColor.a * 0.18)
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    color: Qt.rgba(
                        Theme.foregroundPrimaryColor.r,
                        Theme.foregroundPrimaryColor.g,
                        Theme.foregroundPrimaryColor.b,
                        Theme.foregroundPrimaryColor.a * 0.12)
                    width: Math.max(0, editor.xFromFrequency(editor.leftFrequencyHz))
                    z: 1
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    color: Qt.rgba(
                        Theme.foregroundPrimaryColor.r,
                        Theme.foregroundPrimaryColor.g,
                        Theme.foregroundPrimaryColor.b,
                        Theme.foregroundPrimaryColor.a * 0.12)
                    width: Math.max(
                        0, parent.width
                            - editor.xFromFrequency(editor.rightFrequencyHz))
                    z: 1
                }

                Rectangle {
                    id: leftEdge

                    x: Math.round(editor.xFromFrequency(editor.leftFrequencyHz)
                                  - width * 0.5)
                    width: 2
                    height: parent.height
                    color: Theme.foregroundPrimaryColor
                    z: 2

                    MouseArea {
                        id: leftEdgeMouseArea

                        anchors.centerIn: parent
                        width: 14
                        height: parent.height
                        activeFocusOnTab: true
                        cursorShape: Qt.SizeHorCursor
                        preventStealing: true
                        Accessible.role: Accessible.Slider
                        Accessible.name: qsTr("Left Band Edge")
                        Accessible.description: qsTr("Adjusts frequency and bandwidth")
                        onPositionChanged: mouse => {
                            if (!pressed)
                                return
                            const position = mapToItem(graphArea, mouse.x, mouse.y)
                            editor.previewLeftEdge(editor.frequencyFromX(position.x))
                        }
                        onReleased: editor.effectsUnit.commitPreview()
                        onCanceled: editor.effectsUnit.commitPreview()
                        Keys.onPressed: event => {
                            const fine = event.modifiers & Qt.ShiftModifier
                            const ratio = Math.pow(2, fine ? 1 / 192 : 1 / 48)
                            if (event.key === Qt.Key_Left) {
                                editor.previewLeftEdge(editor.leftFrequencyHz / ratio)
                            } else if (event.key === Qt.Key_Right) {
                                editor.previewLeftEdge(editor.leftFrequencyHz * ratio)
                            } else {
                                return
                            }
                            editor.effectsUnit.commitPreview()
                            event.accepted = true
                        }
                    }
                }

                Rectangle {
                    id: rightEdge

                    x: Math.round(editor.xFromFrequency(editor.rightFrequencyHz)
                                  - width * 0.5)
                    width: 2
                    height: parent.height
                    color: Theme.foregroundPrimaryColor
                    z: 2

                    MouseArea {
                        id: rightEdgeMouseArea

                        anchors.centerIn: parent
                        width: 14
                        height: parent.height
                        activeFocusOnTab: true
                        cursorShape: Qt.SizeHorCursor
                        preventStealing: true
                        Accessible.role: Accessible.Slider
                        Accessible.name: qsTr("Right Band Edge")
                        Accessible.description: qsTr("Adjusts frequency and bandwidth")
                        onPositionChanged: mouse => {
                            if (!pressed)
                                return
                            const position = mapToItem(graphArea, mouse.x, mouse.y)
                            editor.previewRightEdge(editor.frequencyFromX(position.x))
                        }
                        onReleased: editor.effectsUnit.commitPreview()
                        onCanceled: editor.effectsUnit.commitPreview()
                        Keys.onPressed: event => {
                            const fine = event.modifiers & Qt.ShiftModifier
                            const ratio = Math.pow(2, fine ? 1 / 192 : 1 / 48)
                            if (event.key === Qt.Key_Left) {
                                editor.previewRightEdge(editor.rightFrequencyHz / ratio)
                            } else if (event.key === Qt.Key_Right) {
                                editor.previewRightEdge(editor.rightFrequencyHz * ratio)
                            } else {
                                return
                            }
                            editor.effectsUnit.commitPreview()
                            event.accepted = true
                        }
                    }
                }
            }
        }

        Item {
            id: levelMeterArea

            readonly property double maximumDb: 0
            readonly property double minimumDb: -48
            readonly property double normalizedThreshold: Math.min(Math.max(
                ((editor.effectsUnit?.thresholdDb ?? -30) - minimumDb)
                    / (maximumDb - minimumDb), 0), 1)

            Layout.fillHeight: true
            implicitWidth: levelMeterLayout.implicitWidth

            RowLayout {
                id: levelMeterLayout

                anchors.fill: parent
                LayoutMirroring.enabled: false
                LayoutMirroring.childrenInherit: true
                spacing: 2

                DeEsserLevelMeter {
                    Layout.fillHeight: true
                    Accessible.name: qsTr("Left Sibilance Band Level")
                    value: editor.effectsUnit?.leftBandLevelDb ?? -96
                }
                DeEsserLevelMeter {
                    Layout.fillHeight: true
                    Accessible.name: qsTr("Left Gain Reduction")
                    from: 0
                    reversed: true
                    segmented: false
                    to: 12
                    value: editor.effectsUnit?.leftGainReductionDb ?? 0
                }
                DeEsserLevelMeter {
                    Layout.fillHeight: true
                    Accessible.name: qsTr("Right Gain Reduction")
                    from: 0
                    reversed: true
                    segmented: false
                    to: 12
                    value: editor.effectsUnit?.rightGainReductionDb ?? 0
                }
                DeEsserLevelMeter {
                    Layout.fillHeight: true
                    Accessible.name: qsTr("Right Sibilance Band Level")
                    value: editor.effectsUnit?.rightBandLevelDb ?? -96
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
        columns: 3
        columnSpacing: 8

        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Frequency")
            unit: qsTr("Hz")
            parameterValue: editor.effectsUnit?.frequencyHz ?? 6000
            minimum: editor.minimumFrequencyHz
            maximum: editor.maximumFrequencyHz
            defaultValue: 6000
            decimals: 1
            positionFromValue: value => editor.logarithmicPosition(value, minimum, maximum)
            valueFromPosition: position => editor.logarithmicValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewFrequencyHz(value)
            setValue: value => editor.effectsUnit.setFrequencyHz(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Bandwidth")
            unit: qsTr("Hz")
            parameterValue: editor.effectsUnit?.bandwidthHz ?? 3000
            minimum: editor.minimumBandwidthHz
            maximum: editor.maximumBandwidthHz
            defaultValue: 3000
            decimals: 1
            positionFromValue: value => editor.logarithmicPosition(value, minimum, maximum)
            valueFromPosition: position => editor.logarithmicValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewBandwidthHz(value)
            setValue: value => editor.effectsUnit.setBandwidthHz(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Threshold")
            unit: qsTr("dB")
            parameterValue: editor.effectsUnit?.thresholdDb ?? -30
            minimum: -96
            maximum: 0
            defaultValue: -30
            decimals: 1
            positionFromValue: value => editor.linearPosition(value, minimum, maximum)
            valueFromPosition: position => editor.linearValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewThresholdDb(value)
            setValue: value => editor.effectsUnit.setThresholdDb(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
    }

    CheckBox {
        Layout.fillWidth: true
        text: qsTr("Output Sibilance Only")
        checked: editor.effectsUnit?.outputSibilanceOnly ?? false
        onToggled: editor.effectsUnit.setOutputSibilanceOnly(checked)
    }
}
