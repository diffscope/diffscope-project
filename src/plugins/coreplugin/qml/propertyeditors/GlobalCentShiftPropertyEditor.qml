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
    title: qsTr("Global Cent Shift")
    ColumnLayout {
        width: parent.width
        IntegerPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model.global ?? null
            useSlider: true
            key: "centShift"
            label: qsTr("Cent Shift")
            from: -50
            to: 50
            transactionName: qsTr("Editing cent shift")
        }
    }
}