// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Audio

Dialog {
    id: dialog

    required property MidiRecordingAddOn addOn
    property int initialDenominator: 8
    property int tuplet: 2

    function finish(decision) {
        addOn.finishRecording(decision, durationBox.currentValue, dialog.tuplet, trimOverlapsBox.checked, dialog)
    }

    title: qsTr("Quantize MIDI Recording")
    width: Math.min(440, parent ? parent.width - 24 : 440)
    modal: true
    closePolicy: Popup.CloseOnEscape
    onOpened: durationBox.forceActiveFocus()
    onRejected: finish(MidiRecordingAddOn.Keep)
    onClosed: finish(MidiRecordingAddOn.Keep)

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            id: durationLabel
            text: qsTr("Quantization")
        }
        ComboBox {
            id: durationBox
            Layout.fillWidth: true
            Accessible.name: durationLabel.text
            textRole: "text"
            valueRole: "value"
            Component.onCompleted: currentIndex = Qt.binding(() => indexOfValue(dialog.initialDenominator))
            model: [
                {text: qsTr("Whole note"), value: 1},
                {text: qsTr("Half note"), value: 2},
                {text: qsTr("Quarter note"), value: 4},
                {text: qsTr("8th note"), value: 8},
                {text: qsTr("16th note"), value: 16},
                {text: qsTr("32nd note"), value: 32},
                {text: qsTr("64th note"), value: 64},
                {text: qsTr("128th note"), value: 128}
            ]
        }
        RowLayout {
            spacing: 12
            CheckBox {
                text: qsTr("Triplet")
                checked: dialog.tuplet === 3
                onClicked: dialog.tuplet = checked ? 3 : 2
            }
            CheckBox {
                text: qsTr("Quintuplet")
                checked: dialog.tuplet === 5
                onClicked: dialog.tuplet = checked ? 5 : 2
            }
        }
        CheckBox {
            id: trimOverlapsBox
            Layout.fillWidth: true
            text: qsTr("Remove overlaps between recorded notes")
            checked: true
            DescriptiveText.toolTip: qsTr("Shorten earlier notes to the start of the next note. If notes start together, keep the last recorded note.")
            DescriptiveText.bindAccessibleDescription: true
        }
    }

    footer: Item {
        implicitWidth: buttons.implicitWidth + 24
        implicitHeight: buttons.implicitHeight + 24
        RowLayout {
            id: buttons
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12
            Button {
                text: qsTr("Discard")
                ThemedItem.controlType: SVS.CT_Error
                onClicked: dialog.finish(MidiRecordingAddOn.Discard)
            }
            Item {
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("OK")
                ThemedItem.controlType: SVS.CT_Accent
                onClicked: dialog.finish(MidiRecordingAddOn.Quantize)
            }
            Button {
                text: qsTr("Cancel")
                onClicked: dialog.reject()
            }
        }
    }
}
