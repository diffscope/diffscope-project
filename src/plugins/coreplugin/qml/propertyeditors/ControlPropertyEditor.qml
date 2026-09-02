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
    title: qsTr("Control")
    ColumnLayout {
        id: columnLayout
        width: parent.width
        BooleanPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "mute"
            label: qsTr("Mute")
            transactionName: qsTr("Toggling mute")
        }
        BooleanPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            visible: groupBox.propertyMapper && ("solo" in groupBox.propertyMapper)
            key: "solo"
            label: qsTr("Solo")
            transactionName: qsTr("Toggling solo")
        }
        BooleanPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            visible: groupBox.propertyMapper && ("record" in groupBox.propertyMapper)
            key: "record"
            label: qsTr("Record")
            transactionName: qsTr("Toggling record")
        }
        DoublePropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
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
            propertyMapper: groupBox.propertyMapper
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
