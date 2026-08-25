// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core
import DiffScope.Audio
import DiffScope.EqualizerEffectsUnit

ColumnLayout {
    id: editor

    required property EqualizerEffectsUnit effectsUnit

    readonly property double minimumFrequencyHz: 20
    readonly property double maximumFrequencyHz: 20000
    readonly property double minimumGainDb: -24
    readonly property double maximumGainDb: 24

    implicitWidth: 360
    spacing: 4

    function logarithmicPosition(value, minimum, maximum) {
        return Math.log(value / minimum) / Math.log(maximum / minimum)
    }

    function logarithmicValue(position, minimum, maximum) {
        return minimum * Math.pow(maximum / minimum, position)
    }

    function linearPosition(value, minimum, maximum) {
        return (value - minimum) / (maximum - minimum)
    }

    function linearValue(position, minimum, maximum) {
        return minimum + position * (maximum - minimum)
    }

    function frequencyFromX(x) {
        const position = Math.min(Math.max(x / Math.max(graphArea.width, 1), 0), 1)
        return logarithmicValue(position, minimumFrequencyHz, maximumFrequencyHz)
    }

    function xFromFrequency(frequencyHz) {
        return logarithmicPosition(
            Math.min(Math.max(frequencyHz, minimumFrequencyHz), maximumFrequencyHz),
            minimumFrequencyHz, maximumFrequencyHz) * graphArea.width
    }

    function gainFromY(y) {
        const position = 1 - Math.min(Math.max(y / Math.max(graphArea.height, 1), 0), 1)
        return linearValue(position, minimumGainDb, maximumGainDb)
    }

    function yFromGain(gainDb) {
        return (1 - linearPosition(
                    Math.min(Math.max(gainDb, minimumGainDb), maximumGainDb),
                    minimumGainDb, maximumGainDb)) * graphArea.height
    }

    Rectangle {
        id: graphFrame

        Layout.fillWidth: true
        Layout.minimumHeight: 150
        Layout.preferredHeight: 150
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
            Accessible.name: qsTr("Equalizer response and output spectrum")

            EqualizerGraph {
                anchors.fill: parent
                effectsUnit: editor.effectsUnit
                guideColor: Theme.borderColor
                responseColor: Theme.foregroundPrimaryColor
                spectrumColor: Qt.rgba(
                    Theme.accentColor.r, Theme.accentColor.g,
                    Theme.accentColor.b, Theme.accentColor.a * 0.18)
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onDoubleClicked: mouse => {
                    editor.effectsUnit.addBandAt(
                        editor.frequencyFromX(mouse.x), editor.gainFromY(mouse.y))
                }
            }

            Repeater {
                model: editor.effectsUnit?.bands ?? null

                delegate: Item {
                    id: controlPoint

                    required property int index
                    required property double frequencyHz
                    required property double gainDb
                    required property double q

                    readonly property bool selected: editor.effectsUnit?.currentIndex === index
                    readonly property color pointColor: {
                        const colors = CoreInterface.trackColorSchema.colors
                        if (!colors || colors.length === 0)
                            return Theme.accentColor
                        const count = colors.length
                        const sequenceIndex = index % count
                        const colorIndex = sequenceIndex % 2
                            ? Math.floor((count + sequenceIndex) / 2)
                            : Math.floor(sequenceIndex / 2)
                        return colors[colorIndex]
                    }

                    x: editor.xFromFrequency(frequencyHz) - width * 0.5
                    y: editor.yFromGain(gainDb) - height * 0.5
                    z: 2
                    width: 20
                    height: 20
                    activeFocusOnTab: true
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Band %1").arg(
                        Number(index + 1).toLocaleString(Qt.locale(), "f", 0))
                    Accessible.description: qsTr("Frequency %1 Hz, gain %2 dB, Q %3")
                        .arg(Number(frequencyHz).toLocaleString(Qt.locale(), "f", 1))
                        .arg(Number(gainDb).toLocaleString(Qt.locale(), "f", 1))
                        .arg(Number(q).toLocaleString(Qt.locale(), "f", 2))
                    Accessible.onPressAction: editor.effectsUnit.selectBand(index)

                    Rectangle {
                        anchors.centerIn: parent
                        width: controlPoint.selected ? 13 : 11
                        height: width
                        radius: width * 0.5
                        color: controlPoint.pointColor
                        border.color: controlPoint.selected
                            ? Theme.foregroundPrimaryColor
                            : Theme.borderColor
                        border.width: controlPoint.selected ? 2 : 1
                    }

                    MouseArea {
                        id: pointMouseArea

                        property point pressPosition
                        property bool dragging: false

                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        hoverEnabled: true
                        preventStealing: true
                        onPressed: mouse => {
                            controlPoint.forceActiveFocus()
                            if (mouse.button === Qt.LeftButton) {
                                editor.effectsUnit.selectBand(controlPoint.index)
                                pressPosition = mapToItem(graphArea, mouse.x, mouse.y)
                                dragging = false
                            }
                        }
                        onPositionChanged: mouse => {
                            if (!(pressedButtons & Qt.LeftButton))
                                return
                            const graphPosition = mapToItem(graphArea, mouse.x, mouse.y)
                            const distance = Math.hypot(
                                graphPosition.x - pressPosition.x,
                                graphPosition.y - pressPosition.y)
                            if (!dragging && distance >= 2)
                                dragging = true
                            if (dragging) {
                                editor.effectsUnit.previewBandPosition(
                                    controlPoint.index,
                                    editor.frequencyFromX(graphPosition.x),
                                    editor.gainFromY(graphPosition.y))
                            }
                        }
                        onReleased: mouse => {
                            if (mouse.button === Qt.LeftButton && dragging)
                                editor.effectsUnit.commitPreview()
                            dragging = false
                        }
                        onCanceled: {
                            if (dragging)
                                editor.effectsUnit.commitPreview()
                            dragging = false
                        }
                        onClicked: mouse => {
                            if (mouse.button === Qt.RightButton)
                                editor.effectsUnit.removeBand(controlPoint.index)
                        }
                        onWheel: wheel => {
                            editor.effectsUnit.selectBand(controlPoint.index)
                            const steps = wheel.angleDelta.y / 120
                            editor.effectsUnit.previewBandQ(
                                controlPoint.index,
                                controlPoint.q * Math.pow(2, steps / 8))
                            wheelCommitTimer.restart()
                            wheel.accepted = true
                        }
                    }

                    Timer {
                        id: wheelCommitTimer
                        interval: 150
                        onTriggered: editor.effectsUnit.commitPreview()
                    }

                    Keys.onPressed: event => {
                        editor.effectsUnit.selectBand(controlPoint.index)
                        const fine = event.modifiers & Qt.ShiftModifier
                        if (event.key === Qt.Key_Left) {
                            editor.effectsUnit.setCurrentFrequencyHz(
                                controlPoint.frequencyHz / Math.pow(2, fine ? 1 / 192 : 1 / 48))
                        } else if (event.key === Qt.Key_Right) {
                            editor.effectsUnit.setCurrentFrequencyHz(
                                controlPoint.frequencyHz * Math.pow(2, fine ? 1 / 192 : 1 / 48))
                        } else if (event.key === Qt.Key_Up) {
                            editor.effectsUnit.setCurrentGainDb(
                                controlPoint.gainDb + (fine ? 0.1 : 0.5))
                        } else if (event.key === Qt.Key_Down) {
                            editor.effectsUnit.setCurrentGainDb(
                                controlPoint.gainDb - (fine ? 0.1 : 0.5))
                        } else if (event.key === Qt.Key_PageUp) {
                            editor.effectsUnit.setCurrentQ(
                                controlPoint.q * Math.pow(2, 1 / 8))
                        } else if (event.key === Qt.Key_PageDown) {
                            editor.effectsUnit.setCurrentQ(
                                controlPoint.q / Math.pow(2, 1 / 8))
                        } else if (event.key === Qt.Key_Delete) {
                            editor.effectsUnit.removeBand(controlPoint.index)
                        } else if (event.key === Qt.Key_Return
                                   || event.key === Qt.Key_Enter
                                   || event.key === Qt.Key_Space) {
                            editor.effectsUnit.selectBand(controlPoint.index)
                        } else {
                            return
                        }
                        event.accepted = true
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 2

        ToolButton {
            text: qsTr("Previous")
            display: AbstractButton.IconOnly
            icon.source: "image://fluent-system-icons/chevron_left"
            enabled: editor.effectsUnit?.bandCount > 0
            onClicked: editor.effectsUnit.selectPreviousBand()
            ToolTip.visible: hovered
            ToolTip.text: text
        }
        ToolButton {
            text: qsTr("Next")
            display: AbstractButton.IconOnly
            icon.source: "image://fluent-system-icons/chevron_right"
            enabled: editor.effectsUnit?.bandCount > 0
            onClicked: editor.effectsUnit.selectNextBand()
            ToolTip.visible: hovered
            ToolTip.text: text
        }
        ToolButton {
            text: qsTr("Add")
            display: AbstractButton.IconOnly
            icon.source: "image://fluent-system-icons/add"
            enabled: editor.effectsUnit?.canAddBand ?? false
            onClicked: editor.effectsUnit.addBand()
            ToolTip.visible: hovered
            ToolTip.text: text
        }
        ToolButton {
            text: qsTr("Delete")
            display: AbstractButton.IconOnly
            icon.source: "image://fluent-system-icons/delete"
            enabled: editor.effectsUnit?.hasCurrentBand ?? false
            onClicked: editor.effectsUnit.removeCurrentBand()
            ToolTip.visible: hovered
            ToolTip.text: text
        }

        ToolBarContainerSeparator {
        }

        ToolButton {
            text: (editor.effectsUnit?.currentEnabled ?? false)
                ? qsTr("Disable")
                : qsTr("Enable")
            display: AbstractButton.IconOnly
            icon.source: "image://fluent-system-icons/power"
            checkable: true
            checked: editor.effectsUnit?.currentEnabled ?? false
            enabled: editor.effectsUnit?.hasCurrentBand ?? false
            ThemedItem.controlType: checked ? SVS.CT_Accent : SVS.CT_Normal
            onToggled: editor.effectsUnit.setCurrentEnabled(checked)
            ToolTip.visible: hovered
            ToolTip.text: text
        }
        ToolButton {
            text: qsTr("Solo")
            display: AbstractButton.IconOnly
            icon.source: "image://fluent-system-icons/solo"
            checkable: true
            checked: editor.effectsUnit?.currentSolo ?? false
            enabled: editor.effectsUnit?.hasCurrentBand ?? false
            ThemedItem.controlType: checked ? SVS.CT_Accent : SVS.CT_Normal
            onToggled: editor.effectsUnit.setCurrentSolo(checked)
            ToolTip.visible: hovered
            ToolTip.text: text
        }

        Item {
            Layout.fillWidth: true
        }

        ButtonGroup {
            id: typeButtonGroup
        }
        ToolButton {
            text: qsTr("Bell")
            display: AbstractButton.IconOnly
            icon.source: "image://fluent-system-icons/filter_bell"
            checkable: true
            checked: (editor.effectsUnit?.hasCurrentBand ?? false)
                && editor.effectsUnit.currentType === EqualizerEffectsUnit.Bell
            enabled: editor.effectsUnit?.hasCurrentBand ?? false
            ButtonGroup.group: typeButtonGroup
            ThemedItem.controlType: checked ? SVS.CT_Accent : SVS.CT_Normal
            onClicked: editor.effectsUnit.setCurrentType(EqualizerEffectsUnit.Bell)
            ToolTip.visible: hovered
            ToolTip.text: text
        }
        ToolButton {
            text: qsTr("Low Shelf")
            display: AbstractButton.IconOnly
            icon.source: "image://fluent-system-icons/filter_shelving_lo"
            checkable: true
            checked: (editor.effectsUnit?.hasCurrentBand ?? false)
                && editor.effectsUnit.currentType === EqualizerEffectsUnit.LowShelf
            enabled: editor.effectsUnit?.hasCurrentBand ?? false
            ButtonGroup.group: typeButtonGroup
            ThemedItem.controlType: checked ? SVS.CT_Accent : SVS.CT_Normal
            onClicked: editor.effectsUnit.setCurrentType(EqualizerEffectsUnit.LowShelf)
            ToolTip.visible: hovered
            ToolTip.text: text
        }
        ToolButton {
            text: qsTr("High Shelf")
            display: AbstractButton.IconOnly
            icon.source: "image://fluent-system-icons/filter_shelving_hi"
            checkable: true
            checked: (editor.effectsUnit?.hasCurrentBand ?? false)
                && editor.effectsUnit.currentType === EqualizerEffectsUnit.HighShelf
            enabled: editor.effectsUnit?.hasCurrentBand ?? false
            ButtonGroup.group: typeButtonGroup
            ThemedItem.controlType: checked ? SVS.CT_Accent : SVS.CT_Normal
            onClicked: editor.effectsUnit.setCurrentType(EqualizerEffectsUnit.HighShelf)
            ToolTip.visible: hovered
            ToolTip.text: text
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 3
        columnSpacing: 8

        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            enabled: editor.effectsUnit?.hasCurrentBand ?? false
            label: qsTr("Frequency")
            unit: qsTr("Hz")
            parameterValue: editor.effectsUnit?.currentFrequencyHz ?? 1000
            minimum: editor.minimumFrequencyHz
            maximum: editor.maximumFrequencyHz
            defaultValue: 1000
            decimals: 1
            positionFromValue: value => editor.logarithmicPosition(value, minimum, maximum)
            valueFromPosition: position => editor.logarithmicValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewCurrentFrequencyHz(value)
            setValue: value => editor.effectsUnit.setCurrentFrequencyHz(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            enabled: editor.effectsUnit?.hasCurrentBand ?? false
            label: qsTr("Gain")
            unit: qsTr("dB")
            parameterValue: editor.effectsUnit?.currentGainDb ?? 0
            minimum: editor.minimumGainDb
            maximum: editor.maximumGainDb
            defaultValue: 0
            decimals: 1
            positionFromValue: value => editor.linearPosition(value, minimum, maximum)
            valueFromPosition: position => editor.linearValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewCurrentGainDb(value)
            setValue: value => editor.effectsUnit.setCurrentGainDb(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            enabled: editor.effectsUnit?.hasCurrentBand ?? false
            label: qsTr("Q")
            unit: ""
            parameterValue: editor.effectsUnit?.currentQ ?? 1
            minimum: 0.1
            maximum: 24
            defaultValue: 1
            decimals: 2
            positionFromValue: value => editor.logarithmicPosition(value, minimum, maximum)
            valueFromPosition: position => editor.logarithmicValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewCurrentQ(value)
            setValue: value => editor.effectsUnit.setCurrentQ(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
    }
}
