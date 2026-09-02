// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Dialog {
    id: dialog

    property double stretch: 1
    property double pitch: 0
    property int formantMode: 0
    property double formantShift: 0
    property bool linkChannels: true

    width: 420
    title: qsTr("Stretch and Shift Pitch")
    standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

    onAboutToShow: stretchSpinBox.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2
        columnSpacing: 12
        rowSpacing: 8

        Label {
            id: stretchLabel
            text: qsTr("Stretch")
        }
        RowLayout {
            Layout.fillWidth: true
            DoubleSpinBox {
                id: stretchSpinBox
                decimals: 2

                Accessible.labelledBy: stretchLabel
                Accessible.name: stretchLabel.text
                Layout.fillWidth: true
                from: 10
                to: 1000
                value: dialog.stretch * 100
                editable: true
                onValueModified: dialog.stretch = value / 100
            }
            Label {
                text: qsTr("%")
            }
        }

        Label {
            id: pitchLabel
            text: qsTr("Pitch")
        }
        RowLayout {
            Layout.fillWidth: true
            DoubleSpinBox {
                id: pitchSpinBox
                decimals: 2

                Accessible.labelledBy: pitchLabel
                Accessible.name: pitchLabel.text
                Layout.fillWidth: true
                from: -24
                to: 24
                value: dialog.pitch
                editable: true
                onValueModified: dialog.pitch = value
            }
            Label {
                text: qsTr("semitones")
            }
        }


        Label {
            id: formantModeLabel
            text: qsTr("Formant Mode")
        }
        ComboBox {
            id: formantModeComboBox
            Accessible.labelledBy: formantModeLabel
            Accessible.name: formantModeLabel.text
            Layout.fillWidth: true
            model: [qsTr("Shift with pitch"), qsTr("Preserve"), qsTr("Custom")]
            currentIndex: dialog.formantMode
            onActivated: (index) => dialog.formantMode = index
        }

        Label {
            id: formantShiftLabel
            enabled: formantModeComboBox.currentIndex === 2
            text: qsTr("Formant Shift")
        }
        RowLayout {
            Layout.fillWidth: true
            DoubleSpinBox {
                id: formantShiftSpinBox
                decimals: 2

                Accessible.labelledBy: formantShiftLabel
                Accessible.name: formantShiftLabel.text
                Layout.fillWidth: true
                enabled: formantModeComboBox.currentIndex === 2
                from: -24
                to: 24
                value: dialog.formantShift
                editable: true
                onValueModified: dialog.formantShift = value
            }
            Label {
                text: qsTr("semitones")
            }
        }

        CheckBox {
            Layout.columnSpan: 2
            text: qsTr("Link Channels")
            checked: dialog.linkChannels
            onToggled: dialog.linkChannels = checked
        }
    }
}
