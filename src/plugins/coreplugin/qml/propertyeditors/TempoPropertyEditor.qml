import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

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
            label: qsTr("Position")
            columnItem: TextField {
                text: groupBox.propertyMapper?.pos !== undefined ? GlobalHelper.musicTimelineTextFromValue(groupBox.windowHandle?.projectTimeline.musicTimeline ?? null, groupBox.propertyMapper.pos, 1, 1, 3) : ""
                readOnly: true
                ThemedItem.flat: true
            }
        }
        IntegerPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "value"
            label: qsTr("Value")
            from: 10
            to: 1000
            transactionName: qsTr("Editing tempo")
        }
    }
}