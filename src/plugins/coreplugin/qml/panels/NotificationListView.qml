import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

Item {
    id: view

    required property QtObject notificationModel
    readonly property int count: notificationItemModel.count

    Component {
        id: notificationComponent
        BubbleNotification {
            width: parent?.width ?? 0
        }
    }

    ObjectModel {
        id: notificationItemModel
    }

    Component.onCompleted: () => {
        [...notificationModel.messages()].forEach((message, index) => {
            notificationItemModel.insert(notificationItemModel.count - index, notificationComponent.createObject(view, {
                handle: message.handle
            }))
        })
    }

    Connections {
        target: view.notificationModel
        function onMessageAdded(index, message) {
            notificationItemModel.insert(notificationItemModel.count - index, notificationComponent.createObject(view, {
                handle: message.handle
            }))
        }
        function onMessageRemoved(index, message) {
            const itemIndex = notificationItemModel.count - index - 1
            const notification = notificationItemModel.get(itemIndex)
            notificationItemModel.remove(itemIndex)
            notification.destroy()
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        ColumnLayout {
            width: scrollView.width
            Label {
                Layout.fillWidth: true
                Layout.margins: 8
                visible: notificationItemModel.count === 0
                ThemedItem.foregroundLevel: SVS.FL_Secondary
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: qsTr("No notification")
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
                    model: notificationItemModel
                }
            }
        }
    }
}
