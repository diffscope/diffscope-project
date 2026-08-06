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
            key: "clipStart"
            label: qsTr("Starting offset")
            to: ((groupBox.propertyMapper?.length ?? 0) === 0 ? 2147483647 : groupBox.propertyMapper.length) - (groupBox.propertyMapper?.clipLength ?? 0)
            transactionName: qsTr("Editing clip starting offset")
        }
        MusicTimeOffsetPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "clipLength"
            label: qsTr("Clip length")
            from: 1
            to: ((groupBox.propertyMapper?.length ?? 0) === 0 ? 2147483647 : groupBox.propertyMapper.length) - (groupBox.propertyMapper?.clipStart ?? 0)
            transactionName: qsTr("Editing clip length")
        }
        AbstractPropertyEditorField {
            id: lengthFixedField
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "lengthFixed"
            transactionName: qsTr("Editing clip length")
            FormGroup {
                Layout.fillWidth: true
                label: qsTr("Maximum length")
                rowItem: CheckBox {
                    text: "Fix maximum length"
                    tristate: true
                    checkState: lengthFixedField.propertyMapper?.inactive ? Qt.Unchecked : lengthFixedField.value === undefined ? Qt.PartiallyChecked : lengthFixedField.value ? Qt.Checked : Qt.Unchecked
                    nextCheckState: function() {
                        return checkState === Qt.Checked ? Qt.Unchecked : Qt.Checked
                    }
                    onClicked: lengthFixedField.setValue(checkState === Qt.Checked)
                }
                columnItem: TextField {
                    text: groupBox.propertyMapper?.length === 0 ? qsTr("Limitless") : groupBox.propertyMapper?.length === undefined ? "" : groupBox.propertyMapper.length
                    readOnly: true
                    ThemedItem.flat: true
                }
            }
        }
    }
}
