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
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model.timeline ?? null
            key: "loopEnabled"
            text: qsTr("Enable loop")
            transactionName: qsTr("Toggling loop")
        }
        MusicTimePropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.windowHandle?.projectDocumentContext.document.model.timeline ?? null
            enabled: groupBox.windowHandle?.projectDocumentContext.document.model.timeline.loopEnabled ?? false
            key: "loopStart"
            label: qsTr("Start position")
            to: {
                let timeline = groupBox.windowHandle?.projectDocumentContext.document.model.timeline
                if (!timeline)
                    return 2147483647
                let loopEnd = timeline.loopStart + timeline.loopLength
                return loopEnd - 1
            }
            transactionName: qsTr("Editing loop start position")
        }
        MusicTimePropertyEditorField {
            id: loopEndField
            windowHandle: groupBox.windowHandle
            propertyMapper: QtObject {
                readonly property int _loopEnd: {
                    let timeline = groupBox.windowHandle?.projectDocumentContext.document.model.timeline
                    if (!timeline)
                        return 0
                    return timeline.loopStart + timeline.loopLength
                }
                property int loopEnd: 0
                onLoopEndChanged: () => {
                    let timeline = groupBox.windowHandle?.projectDocumentContext.document.model.timeline
                    if (!timeline)
                        return
                    if (timeline.loopLength !== loopEnd - timeline.loopStart) {
                        timeline.loopLength = loopEnd - timeline.loopStart
                    }
                }
                on_LoopEndChanged: () => {
                    if (loopEnd !==  _loopEnd) {
                        loopEnd = _loopEnd
                    }
                }
            }
            enabled: groupBox.windowHandle?.projectDocumentContext.document.model.timeline.loopEnabled ?? false
            key: "loopEnd"
            label: qsTr("End position")
            from: {
                let timeline = groupBox.windowHandle?.projectDocumentContext.document.model.timeline
                if (!timeline)
                    return 0
                return timeline.loopStart + 1
            }
            transactionName: qsTr("Editing loop end position")
        }
    }
}
