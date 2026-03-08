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
    title: qsTr("Lyric and Pronunciation")

    ColumnLayout {
        width: parent.width

        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "lyric"
            label: qsTr("Lyric")
            transactionName: qsTr("Editing lyric")
        }

        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "language"
            label: qsTr("Language")
            transactionName: qsTr("Editing language")
        }

        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Pronunciation (original)")
            columnItem: TextField {
                text: groupBox.propertyMapper?.pronunciationOriginal === undefined
                      ? qsTr("Multiple values")
                      : groupBox.propertyMapper?.pronunciationOriginal ?? ""
                readOnly: true
                ThemedItem.flat: true
            }
        }

        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "pronunciationEdited"
            label: qsTr("Pronunciation (edited)")
            transactionName: qsTr("Editing pronunciation (edited)")
        }
    }
}