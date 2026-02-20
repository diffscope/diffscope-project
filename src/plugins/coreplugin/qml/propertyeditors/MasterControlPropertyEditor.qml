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
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model.master.control ?? null
            key: "mute"
            label: qsTr("Mute")
            transactionName: qsTr("Toggling mute")
        }
        BooleanPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model.master ?? null
            key: "multiChannelOutput"
            label: qsTr("Multi-channel output")
            transactionName: qsTr("Toggling multi-channel output")
        }
        IntegerPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model.master.control ?? null
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
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model.master.control ?? null
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