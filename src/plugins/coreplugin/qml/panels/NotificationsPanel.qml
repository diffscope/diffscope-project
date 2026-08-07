import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn
    readonly property QtObject notificationManager: addOn?.notificationManager ?? null

    readonly property Component notificationsPanelComponent: ActionDockingPane {
        id: pane
        Connections {
            target: d.addOn
            function on_Diffscope_statusTipTriggered() {
                pane.Docking.dockingView.showPane(pane)
            }
            function onShowPanelRequested() {
                pane.Docking.dockingView.showPane(pane)
            }
        }
        Docking.onVisibleChanged: () => {
            if (Docking.visible)
                d.addOn.deactivateIndicator()
        }
        header: Item {
            anchors.fill: parent
            ToolButton {
                enabled: notificationListView.count !== 0
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "image://fluent-system-icons/dismiss_square_multiple"
                text: qsTr("Clear All")
                onClicked: () => {
                    let messages = d.notificationManager.messages()
                    for (let message of messages) {
                        message.close()
                    }
                }
            }
        }
        NotificationListView {
            id: notificationListView
            anchors.fill: parent
            notificationModel: d.notificationManager
        }
    }
}
