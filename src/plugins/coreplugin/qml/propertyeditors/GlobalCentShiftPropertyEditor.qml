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