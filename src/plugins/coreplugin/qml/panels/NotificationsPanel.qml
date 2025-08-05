import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn
    readonly property QtObject notificationManager: addOn?.notificationManager ?? null

    readonly property Component notificationsPanelComponent: ActionDockingPane {
        Component.onCompleted: () => {
            [...d.notificationManager.messages()].forEach((message, index) => {
                notificationItemsModel.insert(index, bubbleNotificationComponent.createObject(this, {
                    handle: message.handle
                }))
            })
        }
        header: Item {
            anchors.fill: parent
            ToolButton {
                enabled: notificationItemsModel.count !== 0
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "qrc:/diffscope/coreplugin/icons/DismissSquareMultiple16Filled"
                text: qsTr("Clear All")
                onClicked: () => {
                    let messages = d.notificationManager.messages()
                    for (let message of messages) {
                        message.close()
                    }
                }
            }
        }
        Component {
            id: bubbleNotificationComponent
            BubbleNotification {
                width: parent?.width ?? 0
            }
        }
        Connections {
            target: d.notificationManager
            function onMessageAdded(index, message) {
                notificationItemsModel.insert(index, bubbleNotificationComponent.createObject(this, {
                    handle: message.handle
                }))
            }
            function onMessageRemoved(index, message) {
                let o = notificationItemsModel.get(index)
                notificationItemsModel.remove(index)
                o.destroy()
            }
        }
        ObjectModel {
            id: notificationItemsModel
        }
        ScrollView {
            id: scrollView
            anchors.fill: parent
            ColumnLayout {
                width: scrollView.width
                Label {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    visible: notificationItemsModel.count === 0
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: qsTr("No notification")
                }
                Column {
                    id: column
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
                        model: notificationItemsModel
                    }
                }
            }
        }
    }
}