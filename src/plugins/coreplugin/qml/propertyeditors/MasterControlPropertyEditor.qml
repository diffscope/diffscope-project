// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

PropertyEditorGroupBox {
    id: groupBox
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    title: qsTr("Master Control")
    ColumnLayout {
        id: columnLayout
        width: parent.width
        BooleanPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model ?? null
            key: "mute"
            label: qsTr("Mute")
            transactionName: qsTr("Toggling mute")
        }
        BooleanPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model ?? null
            key: "multiChannelOutput"
            label: qsTr("Multi-channel output")
            transactionName: qsTr("Toggling multi-channel output")
        }
        DoublePropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model ?? null
            key: "gain"
            label: qsTr("Gain (dB)")
            useSlider: true
            decimals: 2
            from: SVS.decibelsToGain(-96)
            to: SVS.decibelsToGain(6)
            spinBoxValueFromProperty: v => SVS.gainToDecibels(v)
            propertyFromSpinBoxValue: v => SVS.decibelsToGain(v)
            sliderValueFromProperty: v => SVS.decibelToLinearValue(SVS.gainToDecibels(v)) - SVS.decibelToLinearValue(0)
            propertyFromSliderValue: v => SVS.decibelsToGain(SVS.linearValueToDecibel(v + SVS.decibelToLinearValue(0)))
            transactionName: qsTr("Editing gain")
        }
        DoublePropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model ?? null
            key: "pan"
            label: qsTr("Pan (%)")
            useSlider: true
            decimals: 0
            from: -1
            to: 1
            spinBoxValueFromProperty: v => v * 100
            propertyFromSpinBoxValue: v => v * 0.01
            transactionName: qsTr("Editing pan")
        }
    }
}
