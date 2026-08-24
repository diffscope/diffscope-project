// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.ChannelMapperEffectsUnit
import DiffScope.EffectsUnitManager

ColumnLayout {
    id: editor

    required property ChannelMapperEffectsUnit effectsUnit

    spacing: 8

    function linearPosition(value, minimum, maximum) {
        return (value - minimum) / (maximum - minimum)
    }

    function linearValue(position, minimum, maximum) {
        return minimum + position * (maximum - minimum)
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        ChannelHeader {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            Layout.preferredWidth: 1
            label: qsTr("Left")
        }
        ChannelHeader {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            Layout.preferredWidth: 1
            label: qsTr("Right")
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 4
        columnSpacing: 8

        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Left")
            unit: qsTr("%")
            parameterValue: editor.effectsUnit?.leftLeftMixPercent ?? 100
            minimum: -100
            maximum: 100
            defaultValue: 100
            decimals: 1
            positionFromValue: value => editor.linearPosition(value, minimum, maximum)
            valueFromPosition: position => editor.linearValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewLeftLeftMixPercent(value)
            setValue: value => editor.effectsUnit.setLeftLeftMixPercent(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Right")
            unit: qsTr("%")
            parameterValue: editor.effectsUnit?.leftRightMixPercent ?? 0
            minimum: -100
            maximum: 100
            defaultValue: 0
            decimals: 1
            positionFromValue: value => editor.linearPosition(value, minimum, maximum)
            valueFromPosition: position => editor.linearValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewLeftRightMixPercent(value)
            setValue: value => editor.effectsUnit.setLeftRightMixPercent(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Left")
            unit: qsTr("%")
            parameterValue: editor.effectsUnit?.rightLeftMixPercent ?? 0
            minimum: -100
            maximum: 100
            defaultValue: 0
            decimals: 1
            positionFromValue: value => editor.linearPosition(value, minimum, maximum)
            valueFromPosition: position => editor.linearValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewRightLeftMixPercent(value)
            setValue: value => editor.effectsUnit.setRightLeftMixPercent(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
        EffectsParameterCell {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            label: qsTr("Right")
            unit: qsTr("%")
            parameterValue: editor.effectsUnit?.rightRightMixPercent ?? 100
            minimum: -100
            maximum: 100
            defaultValue: 100
            decimals: 1
            positionFromValue: value => editor.linearPosition(value, minimum, maximum)
            valueFromPosition: position => editor.linearValue(position, minimum, maximum)
            previewValue: value => editor.effectsUnit.previewRightRightMixPercent(value)
            setValue: value => editor.effectsUnit.setRightRightMixPercent(value)
            commitValue: () => editor.effectsUnit.commitPreview()
        }
    }

    component ChannelHeader: RowLayout {
        id: header

        required property string label

        spacing: 2

        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.foregroundSecondaryColor
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: header.label
        }
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.foregroundSecondaryColor
        }
    }
}
