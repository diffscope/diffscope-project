import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

Dialog {
    id: dialog

    property MusicTimeline timeline
    property double tempo
    property int position
    property bool doInsertNew

    title: qsTr("Edit Tempo")

    GridLayout {
        anchors.fill: parent
        columns: 2
        Label {
            text: qsTr("Tempo")
        }
        SpinBox {
            Layout.fillWidth: true
            readonly property int decimals: 2
            from: 10 * Math.pow(10, decimals)
            to: 1000 * Math.pow(10, decimals)
            stepSize: 100
            value: Math.round(dialog.tempo * Math.pow(10, decimals))
            onValueModified: dialog.tempo = value / Math.pow(10, decimals)
            textFromValue: function(value, locale) {
                return Number(value / Math.pow(10, decimals)).toLocaleString(locale, 'f', decimals)
            }
            valueFromText: function(text, locale) {
                return Math.round(Number.fromLocaleString(locale, text) * Math.pow(10, decimals))
            }
            validator: DoubleValidator {
                bottom: 10
                top: 1000
                decimals: 2
            }
        }
        Label {
            text: qsTr("Position")
        }
        MusicTimeSpinBox {
            Layout.fillWidth: true
            timeline: dialog.timeline
            value: dialog.position
            onValueModified: dialog.position = value
        }
        RowLayout {
            Layout.columnSpan: 2
            RadioButton {
                text: qsTr("Modify existing one")
                checked: !dialog.doInsertNew
                onClicked: dialog.doInsertNew = false
            }
            RadioButton {
                text: qsTr("Insert new one")
                checked: dialog.doInsertNew
                onClicked: dialog.doInsertNew = true
            }
        }
    }
    standardButtons: DialogButtonBox.Ok
}