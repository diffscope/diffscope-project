// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ChorusKit.AppCore

import DiffScope.Core

Window {
    id: dialog

    required property QtObject notificationModel

    width: 520
    height: 600
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    title: qsTr("Notifications")

    WindowSystem.windowSystem: CoreInterface.windowSystem
    WindowSystem.id: "org.diffscope.core.notificationlistdialog"

    signal finished()
    onClosing: finished()

    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundPrimaryColor
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        NotificationListView {
            id: notificationListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            notificationModel: dialog.notificationModel
        }

        Button {
            Layout.fillWidth: true
            Layout.margins: 12
            enabled: notificationListView.count !== 0
            text: qsTr("Clear All")
            onClicked: () => {
                for (const message of dialog.notificationModel.messages()) {
                    message.close()
                }
            }
        }
    }
}
