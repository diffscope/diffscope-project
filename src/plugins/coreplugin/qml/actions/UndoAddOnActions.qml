import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

ActionCollection {
    id: d

    required property QtObject addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property QtObject transactionController: addOn?.windowHandle.projectDocumentContext.document.transactionController ?? null

    ActionItem {
        actionId: "org.diffscope.core.edit.undo"
        Action {
            id: undoAction
            enabled: !d.transactionController?.exclusiveToTransaction && d.transactionController?.currentStep > 0
            readonly property Binding binding: Binding {
                undoAction.text: undoAction.enabled ? qsTr("Undo %1").arg(d.transactionController?.stepName(d.transactionController.currentStep - 1)) : qsTr("Undo")
            }
            onTriggered: {
                d.transactionController.currentStep--
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.redo"
        Action {
            id: redoAction
            enabled: !d.transactionController?.exclusiveToTransaction && d.transactionController?.currentStep < d.transactionController?.totalSteps
            readonly property Binding binding: Binding {
                redoAction.text: redoAction.enabled ? qsTr("Redo %1").arg(d.transactionController?.stepName(d.transactionController.currentStep)) : qsTr("Redo")
            }
            onTriggered: {
                d.transactionController.currentStep++
            }
        }
    }
}
