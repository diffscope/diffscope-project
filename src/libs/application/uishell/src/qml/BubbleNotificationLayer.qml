import QtQml
import QtQml.Models
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

    function removeNotification(handle) {
        for (let index = 0; index < notificationItemModel.count; ++index) {
            const notification = notificationItemModel.get(index)
            if (notification.handle !== handle)
                continue
            notificationItemModel.remove(index)
            proxyItemModel.remove(index)
            notification.destroy()
            return
        }
    }

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
            model: ObjectModel {
                id: proxyItemModel
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
        Repeater {
            model: ObjectModel {
                id: notificationItemModel
            }
        }
    }

    Instantiator {
        id: notificationInstantiator
        model: layer.model
        readonly property Component bubbleNotificationComponent: BubbleNotification {
            id: bubbleNotification
            popupLike: true
            property Item proxyItem: Item {
                readonly property BubbleNotification sourceItem: bubbleNotification
                width: sourceItem.width
                height: sourceItem.height
            }
            x: parent ? parent.width - width - proxyItem.x : 0
            y: parent ? parent.height - height - proxyItem.y : 0
        }
        onObjectAdded: (index, object) => {
            const notification = bubbleNotificationComponent.createObject(null, {handle: object})
            notificationItemModel.insert(index, notification)
            proxyItemModel.insert(index, notification.proxyItem)
        }
        onObjectRemoved: (index, object) => layer.removeNotification(object)
    }

    Component.onDestruction: () => {
        // Clear delegates while the sibling ObjectModels are still alive.
        notificationInstantiator.active = false
    }
}
