// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick

import DiffScope.Notifications

QtObject {
    required property QtObject addOn

    readonly property Window window: addOn.windowHandle.window
    readonly property NotificationPresentationModel bubbleModel: NotificationPresentationModel {
        notifications: addOn.windowHandle.notifications.bubbleModel
    }

    Component.onCompleted: window.bubbleNotificationsModel = bubbleModel
}
