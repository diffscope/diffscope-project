// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

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
        preferredSize: Qt.size(400, 400)

        badgeNumber: notificationListView.count
        badgeType: {
            if (pane.badgeNumber === 0)
                return SVS.CT_Normal

            let type = SVS.CT_Normal
            for (const message of d.notificationManager?.messages() ?? []) {
                const icon = message.icon
                if (icon === SVS.Critical)
                    return SVS.CT_Error

                if (icon === SVS.Warning)
                    type = SVS.CT_Warning
                else if ((icon === SVS.Success || icon === SVS.Tip) && type === SVS.CT_Normal)
                    type = SVS.CT_Accent
            }
            return type
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
