import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

FormGroup {
    id: d
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    required property string key
    required property string transactionName
    property int from: 0
    property int to: 2147483647
    Layout.fillWidth: true
    property int transactionId: 0
    function beginTransaction() {
        let a = d.windowHandle.projectDocumentContext.document.transactionController.beginTransaction()
        if (a) {
            transactionId = a
            return true
        }
        return false
    }
    function commitTransaction() {
        d.windowHandle.projectDocumentContext.document.transactionController.commitTransaction(transactionId, d.transactionName)
        transactionId = 0
    }
    columnItem: MusicTimeSpinBox {
        id: control
        timeline: d.windowHandle?.projectTimeline.musicTimeline ?? null
        value: d.propertyMapper?.[d.key] ?? 0
        from: d.from
        to: d.to
        contentItem.visible: d.propertyMapper?.[d.key] !== undefined
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

