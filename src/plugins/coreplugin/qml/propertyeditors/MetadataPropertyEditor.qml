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
    title: qsTr("Metadata")
    ColumnLayout {
        width: parent.width
        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Path")
            rowItem: ToolButton {
                icon.source: "image://fluent-system-icons/open"
                display: AbstractButton.IconOnly
                text: qsTr("Reveal in %1").arg(DesktopServices.fileManagerName)
                enabled: Boolean(groupBox.windowHandle?.projectDocumentContext.fileLocker?.path)
                onClicked: () => {
                    DesktopServices.reveal(groupBox.windowHandle.projectDocumentContext.fileLocker.path)
                }
            }
            columnItem: TextEdit {
                readOnly: true
                color: Theme.foregroundColor(ThemedItem.foregroundLevel)
                font: Theme.font
                Accessible.role: Accessible.StaticText
                Accessible.name: text
                selectionColor: Theme.accentColor
                text: groupBox.windowHandle?.projectDocumentContext.fileLocker?.path || qsTr("Unspecified")

            }
        }
        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model.global ?? null
            key: "name"
            label: qsTr("Title")
            transactionName: qsTr("Editing title")
        }
        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model.global ?? null
            key: "author"
            label: qsTr("Author")
            transactionName: qsTr("Editing author")
        }
    }
}