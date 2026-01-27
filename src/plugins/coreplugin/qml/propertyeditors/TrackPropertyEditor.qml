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
        label: qsTr("Name")
        columnItem: TextField {

        }
    }
    FormGroup {
        Layout.fillWidth: true
        label: qsTr("Color")
    }
}