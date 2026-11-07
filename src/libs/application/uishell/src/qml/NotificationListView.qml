// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

Item {
    id: view

    required property var model
    readonly property int count: notificationRepeater.count

    ScrollView {
        id: scrollView
        anchors.fill: parent
        ColumnLayout {
            width: scrollView.width
            Label {
                Layout.fillWidth: true
                Layout.margins: 8
                visible: view.count === 0
                ThemedItem.foregroundLevel: SVS.FL_Secondary
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: qsTr("No notifications")
            }
            Column {
                Layout.fillWidth: true
                Layout.margins: 12
                spacing: 12
                move: Transition {
                    NumberAnimation {
                        properties: "x,y"
                        easing.type: Easing.OutCubic
                        duration: Theme.visualEffectAnimationDuration
                    }
                }
                Repeater {
                    id: notificationRepeater
                    model: view.model
                    delegate: BubbleNotification {
                        required property QtObject notificationHandle
                        handle: notificationHandle
                        width: parent?.width ?? 0
                    }
                }
            }
        }
    }
}
