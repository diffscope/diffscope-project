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
    title: qsTr("Time")

    ColumnLayout {
        width: parent.width

        MusicTimePropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "pos"
            label: qsTr("Onset position")
            transactionName: qsTr("Editing onset position")
        }

        IntegerPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "length"
            label: qsTr("Duration")
            from: 1
            transactionName: qsTr("Editing duration")
        }
    }
}