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
        MusicTimePropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "pos"
            label: qsTr("Position")
            transactionName: qsTr("Moving label")
        }
        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "text"
            label: qsTr("Text")
            transactionName: qsTr("Editing label")
        }
    }
}