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
    title: qsTr("Basic")
    ColumnLayout {
        width: parent.width
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
}