import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

import SVSCraft
import SVSCraft.UIComponents

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

QtObject {
    id: d
    required property QtObject addOn

    readonly property Window window: addOn.windowHandle.window
    readonly property QtObject notificationManager: addOn.notificationManager

    Component.onCompleted: () => {
        [...notificationManager.bubbleMessages()].forEach((message, index) => {
            d.bubbleNotificationsModel.insert(index, message.handle)
        })
        window.bubbleNotificationsModel = bubbleNotificationsModel
    }

    readonly property ObjectModel bubbleNotificationsModel: ObjectModel {

    }


    readonly property Connections notificationManagerConnections: Connections {
        target: notificationManager
        function onMessageAddedToBubbles(index, message) {
            d.bubbleNotificationsModel.insert(index, message.handle)
        }
        function onMessageRemovedFromBubbles(index, message) {
            d.bubbleNotificationsModel.remove(index)
        }
    }

}