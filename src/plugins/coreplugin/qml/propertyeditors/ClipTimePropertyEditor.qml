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
        id: columnLayout
        width: parent.width
        MusicTimePropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "position"
            label: qsTr("Position")
            transactionName: qsTr("Editing clip position")
        }
        MusicTimeOffsetPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "startingOffset"
            label: qsTr("Starting offset")
            to: ((groupBox.propertyMapper?.fullLength ?? 0) === 0 ? 2147483647 : groupBox.propertyMapper.fullLength) - (groupBox.propertyMapper?.clipLength ?? 0)
            transactionName: qsTr("Editing clip starting offset")
        }
        MusicTimeOffsetPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "clipLength"
            label: qsTr("Clip length")
            from: 1
            to: ((groupBox.propertyMapper?.fullLength ?? 0) === 0 ? 2147483647 : groupBox.propertyMapper.fullLength) - (groupBox.propertyMapper?.startingOffset ?? 0)
            transactionName: qsTr("Editing clip length")
        }
        FormGroup {
            label: qsTr("Full length")
            columnItem: TextField {
                text: groupBox.propertyMapper?.fullLength === 0 ? qsTr("Limitless") : groupBox.propertyMapper?.fullLength === undefined ? "" : groupBox.propertyMapper.fullLength
                readOnly: true
                ThemedItem.flat: true
            }
        }
    }
}