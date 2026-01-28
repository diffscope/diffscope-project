import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell

FormGroup {
    id: d
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    required property string key
    required property string transactionName
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
    columnItem: TextField {
        text: d.propertyMapper?.[d.key] ?? ""
        onEditingFinished: () => {
            if (d.propertyMapper[d.key] === text) {
                return
            }
            d.beginTransaction()
            if (!d.transactionId)
                return
            d.propertyMapper[d.key] = text
            d.commitTransaction()
        }
    }
}

