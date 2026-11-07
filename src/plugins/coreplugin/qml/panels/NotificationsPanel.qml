// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Notifications
import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn
    readonly property QtObject notificationViewModel: addOn?.notificationViewModel ?? null
    readonly property NotificationModel notifications: addOn?.windowHandle.notifications ?? null
    readonly property NotificationPresentationModel presentationModel: NotificationPresentationModel {
        notifications: d.notifications
    }

    readonly property Component notificationsPanelComponent: ActionDockingPane {
        id: pane
        preferredSize: Qt.size(400, 400)

        badgeNumber: d.notificationViewModel?.count ?? 0
        badgeType: d.notificationViewModel?.badgeType ?? SVS.CT_Normal

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
                onClicked: d.notifications.closeAll()
            }
        }
        NotificationListView {
            id: notificationListView
            anchors.fill: parent
            model: d.presentationModel
        }
    }
}
