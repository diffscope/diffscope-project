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
        IntegerPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "gain"
            label: qsTr("Gain (dB)")
            useSlider: true
            from: SVS.decibelsToGain(-96)
            to: SVS.decibelsToGain(6)
            spinBoxValueFromProperty: v => Math.round(SVS.gainToDecibels(v) * 10)
            propertyFromSpinBoxValue: v => SVS.decibelsToGain(v / 10)
            sliderValueFromProperty: v => SVS.decibelToLinearValue(SVS.gainToDecibels(v)) - SVS.decibelToLinearValue(0)
            propertyFromSliderValue: v => SVS.decibelsToGain(SVS.linearValueToDecibel(v + SVS.decibelToLinearValue(0)))
            transactionName: qsTr("Editing gain")
            spinBox.textFromValue: function(value, locale) {
                return Number(value / 10).toLocaleString(locale, 'f', 1)
            }
            spinBox.valueFromText: function(text, locale) {
                return Math.round(Number.fromLocaleString(locale, text) * 10)
            }
        }
        IntegerPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "pan"
            label: qsTr("Pan (%)")
            useSlider: true
            from: -1
            to: 1
            spinBoxValueFromProperty: v => v * 100
            propertyFromSpinBoxValue: v => v * 0.01
            transactionName: qsTr("Editing pan")
        }
    }
}