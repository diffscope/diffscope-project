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
    title: qsTr("Loop")
    ColumnLayout {
        width: parent.width
        BooleanPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model ?? null
            key: "loopEnabled"
            label: qsTr("Enable loop")
            transactionName: qsTr("Toggling loop")
        }
        MusicTimePropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model ?? null
            enabled: groupBox.windowHandle?.projectDocumentContext.document.model.loopEnabled ?? false
            key: "loopStart"
            label: qsTr("Start position")
            to: {
                let model = groupBox.windowHandle?.projectDocumentContext.document.model
                if (!model)
                    return 2147483647
                let loopEnd = model.loopStart + model.loopLength
                return loopEnd - 1
            }
            transactionName: qsTr("Editing loop start position")
        }
        MusicTimePropertyEditorField {
            id: loopEndField
            windowHandle: groupBox.windowHandle
            propertyMapper: QtObject {
                readonly property int _loopEnd: {
                    let model = groupBox.windowHandle?.projectDocumentContext.document.model
                    if (!model)
                        return 0
                    return model.loopStart + model.loopLength
                }
                property int loopEnd: 0
                onLoopEndChanged: () => {
                    let model = groupBox.windowHandle?.projectDocumentContext.document.model
                    if (!model)
                        return
                    if (model.loopLength !== loopEnd - model.loopStart) {
                        model.loopLength = loopEnd - model.loopStart
                    }
                }
                on_LoopEndChanged: () => {
                    if (loopEnd !==  _loopEnd) {
                        loopEnd = _loopEnd
                    }
                }
            }
            enabled: groupBox.windowHandle?.projectDocumentContext.document.model.loopEnabled ?? false
            key: "loopEnd"
            label: qsTr("End position")
            from: {
                let model = groupBox.windowHandle?.projectDocumentContext.document.model
                if (!model)
                    return 0
                return model.loopStart + 1
            }
            transactionName: qsTr("Editing loop end position")
        }
    }
}
