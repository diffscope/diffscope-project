// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

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

    readonly property NotificationBubbleModel bubbleNotificationModel: NotificationBubbleModel {
        notificationModel: d.notificationManager
    }

    Component.onCompleted: window.bubbleNotificationsModel = bubbleNotificationModel.model
}
