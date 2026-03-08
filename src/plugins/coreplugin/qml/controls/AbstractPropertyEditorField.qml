import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

ColumnLayout {
    id: d
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    required property string key
    required property string transactionName
    property string label
    readonly property var value: propertyMapper?.[key]
    property int transactionId: 0
    Layout.fillWidth: true
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
    function abortTransaction() {
        d.windowHandle.projectDocumentContext.document.transactionController.abortTransaction(transactionId)
        transactionId = 0
    }
    function setValue(v) {
        if (value === v)
            return
        beginTransaction()
        if (!transactionId)
            return
        propertyMapper[key] = v
        commitTransaction()
    }
}
