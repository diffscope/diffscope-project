// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQml.Models

QtObject {
    id: d

    required property QtObject notificationModel
    readonly property ObjectModel model: ObjectModel {
    }

    Component.onCompleted: () => {
        [...notificationModel.bubbleMessages()].forEach((message, index) => {
            d.model.insert(index, message.handle)
        })
    }

    readonly property Connections notificationModelConnections: Connections {
        target: d.notificationModel
        function onMessageAddedToBubbles(index, message) {
            d.model.insert(index, message.handle)
        }
        function onMessageRemovedFromBubbles(index, message) {
            d.model.remove(index)
        }
    }
}
