import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

AbstractPropertyEditorField {
    id: d
    property int from: 0
    property int to: 127
    property int positionHint: -1
    readonly property MusicPitchSpinBox spinBox: control
    KeySignatureAtSpecifiedPositionHelper {
        id: keySignatureAtSpecifiedPositionHelper
        position: d.positionHint !== -1 ? d.positionHint : (d.windowHandle?.projectTimeline.position ?? 0)
        keySignatureSequence: d.windowHandle?.projectDocumentContext.document.model.timeline.keySignatures
    }
    FormGroup {
        Layout.fillWidth: true
        label: d.label
        columnItem: MusicPitchSpinBox {
            id: control
            accidentalType: keySignatureAtSpecifiedPositionHelper.keySignature?.accidentalType ?? 0
            value: d.value ?? 0
            from: d.from
            to: d.to
            contentItem.visible: d.value !== undefined
            onValueModified: () => {
                if (!helper.buttonPressed) {
                    d.beginTransaction()
                }
                if (!d.transactionId)
                    return
                d.propertyMapper[d.key] = value
                if (!helper.buttonPressed) {
                    d.commitTransaction()
                }
            }
            SpinBoxPressedHelper {
                id: helper
                spinBox: control
                onPressed: d.beginTransaction()
                onReleased: d.commitTransaction()
            }
        }
    }
}
