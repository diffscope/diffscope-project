import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

ColumnLayout {
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    FormGroup {
        Layout.fillWidth: true
        label: qsTr("Position")
        columnItem: MusicTimeSpinBox {

        }
    }
    FormGroup {
        Layout.fillWidth: true
        label: qsTr("Value")
        columnItem: SpinBox {

        }
    }
}