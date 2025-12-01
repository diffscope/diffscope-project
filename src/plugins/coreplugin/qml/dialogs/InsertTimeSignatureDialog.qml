import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

Dialog {
    title: qsTr("Insert Time Signature")
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
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
            }
            Label {
                text: "/"
            }
            ComboBox {
                Accessible.name: qsTr("Denominator")
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
            }
        }
        Label {
            text: qsTr("Position")
        }
        MusicTimeSpinBox {
            Layout.fillWidth: true
        }
        RowLayout {
            Layout.columnSpan: 2
            RadioButton {
                text: qsTr("Insert new")
            }
            RadioButton {
                text: qsTr("Modify existing")
            }
        }
    }
    standardButtons: DialogButtonBox.Ok
}