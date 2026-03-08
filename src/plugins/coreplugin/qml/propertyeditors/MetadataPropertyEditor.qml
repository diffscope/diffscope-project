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
            columnItem: TextField {
                text: groupBox.windowHandle?.projectDocumentContext.fileLocker?.path || qsTr("Unspecified")
                readOnly: true
                ThemedItem.flat: true
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
        IntegerPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model.global ?? null
            useSlider: true
            key: "centShift"
            label: qsTr("Cent shift")
            from: -50
            to: 50
            transactionName: qsTr("Editing cent shift")
        }
        Label {
            text: qsTr("The standard pitch for this cent shift: %L1 Hz").arg(440 * Math.pow(2, (groupBox.windowHandle?.projectDocumentContext.document.model.global.centShift ?? 0) / 1200))
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }
    }
}