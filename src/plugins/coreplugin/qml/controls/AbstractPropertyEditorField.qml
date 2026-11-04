// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

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
        console.warn("The property edit transaction could not be started.")
        d.windowHandle.sendOperationConflictNotification()
        return false
    }
    function commitTransaction() {
        if (!transactionId)
            return
        const id = transactionId
        transactionId = 0
        d.windowHandle.projectDocumentContext.document.transactionController.commitTransaction(id, d.transactionName)
    }
    function abortTransaction() {
        if (!transactionId)
            return
        const id = transactionId
        transactionId = 0
        d.windowHandle?.projectDocumentContext?.document?.transactionController?.abortTransaction(id)
    }
    Component.onDestruction: abortTransaction()
    function setValue(v, continuous = false): bool {
        if (!continuous && value === v)
            return true
        const started = continuous ? transactionId !== 0 : beginTransaction()
        if (!started)
            return false
        propertyMapper[key] = v
        if (!continuous)
            commitTransaction()
        return true
    }
}
