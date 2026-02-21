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
    property int to: 2147483647
    readonly property MusicTimeOffsetSpinBox spinBox: control
    FormGroup {
        Layout.fillWidth: true
        label: d.label
        columnItem: MusicTimeOffsetSpinBox {
            id: control
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
