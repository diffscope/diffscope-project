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
    title: qsTr("Pitch")

    ColumnLayout {
        width: parent.width

        MusicPitchPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "keyNumber"
            label: qsTr("Pitch")
            from: 0
            to: 127
            positionHint: groupBox.propertyMapper?.position === undefined ? -1 : (groupBox.propertyMapper.position + (groupBox.windowHandle?.projectDocumentContext.document.selectionModel.noteSelectionModel.noteSequenceWithSelectedItems?.singingClip.start ?? 0))
            transactionName: qsTr("Editing pitch")
        }

        IntegerPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "centShift"
            label: qsTr("Cent shift")
            from: -50
            to: 50
            transactionName: qsTr("Editing cent shift")
            useSlider: true
        }
    }
}
