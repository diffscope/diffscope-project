// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick

import SVSCraft.UIComponents

import DiffScope.UIShell

Item {
    id: layer

    property var model: null
    property bool enablesAnimation: false
    property real topMargin: 0
    property real bottomMargin: 0
    property real leftMargin: 0
    property real rightMargin: 0

    Flow {
        id: proxyItemFlow
        anchors.fill: parent
        anchors.topMargin: layer.topMargin
        anchors.bottomMargin: layer.bottomMargin
        anchors.leftMargin: layer.leftMargin
        anchors.rightMargin: layer.rightMargin
        spacing: 12
        flow: Flow.TopToBottom
        add: Transition {
            NumberAnimation {
                property: "x"
                from: proxyItemFlow.effectiveLayoutDirection === Qt.LeftToRight ? -360 : proxyItemFlow.width + 360
                easing.type: Easing.OutCubic
                duration: layer.enablesAnimation ? Theme.visualEffectAnimationDuration : 0
            }
        }
        move: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutCubic
                duration: layer.enablesAnimation ? Theme.visualEffectAnimationDuration : 0
            }
        }
        Repeater {
            model: layer.model
            delegate: Item {
                id: proxyItem
                required property QtObject notificationHandle
                width: notification.width
                height: notification.height

                BubbleNotification {
                    id: notification
                    parent: notificationArea
                    handle: proxyItem.notificationHandle
                    popupLike: true
                    x: notificationArea.width - width - proxyItem.x
                    y: notificationArea.height - height - proxyItem.y
                }
            }
        }
    }

    Item {
        id: notificationArea
        anchors.fill: parent
        anchors.topMargin: layer.topMargin
        anchors.bottomMargin: layer.bottomMargin
        anchors.leftMargin: layer.leftMargin
        anchors.rightMargin: layer.rightMargin
    }
}
