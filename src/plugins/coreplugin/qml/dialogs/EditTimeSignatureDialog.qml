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
    property int numerator
    property int denominator
    property int position
    property bool doInsertNew

    title: qsTr("Edit Time Signature")

    GridLayout {
        anchors.fill: parent
        columns: 2
        Label {
            text: qsTr("Time signature")
        }
        RowLayout {
            Layout.fillWidth: true
            SpinBox {
                Accessible.name: qsTr("Numerator")
                from: 1
                to: 2147483647
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
                value: dialog.numerator
                onValueModified: dialog.numerator = value
            }
            Label {
                text: "/"
            }
            ComboBox {
                Accessible.name: qsTr("Denominator")
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
                model: Array.from({length: 8}, (_, i) => Math.pow(2, i)).map(v => ({
                    text: Qt.locale().toString(v),
                    value: v
                }))
                textRole: "text"
                valueRole: "value"
                currentIndex: indexOfValue(dialog.denominator)
                onActivated: (index) => dialog.denominator = valueAt(index)
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
                text: qsTr("Modify existing")
                checked: !dialog.doInsertNew
                onClicked: dialog.doInsertNew = false
            }
            RadioButton {
                text: qsTr("Insert new")
                checked: dialog.doInsertNew
                onClicked: dialog.doInsertNew = true
            }
        }
    }
    standardButtons: DialogButtonBox.Ok
}