import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

CheckBox {
    id: d
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    required property string key
    required property string transactionName
    tristate: true
    checkState: propertyMapper?.inactive ? Qt.Unchecked : propertyMapper?.[key] === undefined ? Qt.PartiallyChecked : propertyMapper[key] ? Qt.Checked : Qt.Unchecked
    nextCheckState: function() {
        return checkState === Qt.Checked ? Qt.Unchecked : Qt.Checked
    }
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
    onClicked: () => {
        beginTransaction()
        if (!transactionId)
            return
        propertyMapper[key] = checkState === Qt.Checked
        commitTransaction()
    }
}