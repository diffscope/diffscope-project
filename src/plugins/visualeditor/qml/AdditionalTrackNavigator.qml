import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

import QActionKit

import DiffScope.Core

ColumnLayout {
    id: d

    required property QtObject additionalTrackLoader
    required property AdditionalTrackPane associatedPane

    spacing: 0

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: Theme.paneSeparatorColor
    }
    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: false
        spacing: 0
        Repeater {
            model: d.additionalTrackLoader?.loadedComponents ?? []
            ColumnLayout {
                id: layout
                required property string modelData
                required property int index
                readonly property Item item: {
                    let a = d.associatedPane.additionalTrackRepeater.count - d.associatedPane.additionalTrackRepeater.count
                    return d.associatedPane.additionalTrackRepeater.itemAt(a + index)?.item ?? null
                }
                readonly property double itemSize: 14
                Layout.fillWidth: true
                spacing: 0
                Item {
                    id: container
                    Layout.fillWidth: true
                    Layout.preferredHeight: layout.item?.height ?? 0
                    readonly property Action moveUpAction: Action {
                        enabled: layout.index !== 0
                        text: qsTr("Move Up")
                        icon.source: "image://fluent-system-icons/arrow_up"
                        onTriggered: d.additionalTrackLoader.moveUp(layout.modelData)
                    }
                    readonly property Action moveDownAction: Action {
                        enabled: layout.index !== (d.additionalTrackLoader?.loadedComponents.length ?? 0) - 1
                        text: qsTr("Move Down")
                        icon.source: "image://fluent-system-icons/arrow_down"
                        onTriggered: d.additionalTrackLoader.moveDown(layout.modelData)
                    }
                    readonly property Action removeAction: Action {
                        text: qsTr("Remove")
                        icon.source: "image://fluent-system-icons/dismiss"
                        onTriggered: d.additionalTrackLoader.removeItem(layout.modelData)
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 2
                        anchors.leftMargin: 4
                        anchors.rightMargin: 4
                        visible: (layout.item?.height ?? 0) >= 12
                        IconLabel {
                            Layout.fillHeight: true
                            icon.height: layout.itemSize
                            icon.width: layout.itemSize
                            spacing: 2
                            icon.source: layout.item?.ActionInstantiator.icon.source ?? ""
                            icon.color: layout.item?.ActionInstantiator.icon.color.valid ? layout.item.ActionInstantiator.icon.color : Theme.foregroundPrimaryColor
                            text: d.additionalTrackLoader?.componentName(layout.modelData) ?? ""
                            color: Theme.foregroundPrimaryColor
                            font.family: Theme.font.family
                            font.pixelSize: layout.itemSize * 0.75
                        }
                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                        ToolButton {
                            implicitWidth: layout.itemSize
                            implicitHeight: layout.itemSize
                            padding: 0
                            visible: hoverHandler.hovered
                            display: AbstractButton.IconOnly
                            action: container.moveUpAction
                        }
                        ToolButton {
                            implicitWidth: layout.itemSize
                            implicitHeight: layout.itemSize
                            padding: 0
                            visible: hoverHandler.hovered
                            display: AbstractButton.IconOnly
                            action: container.moveDownAction
                        }
                        ToolButton {
                            implicitWidth: layout.itemSize
                            implicitHeight: layout.itemSize
                            padding: 1
                            visible: hoverHandler.hovered
                            display: AbstractButton.IconOnly
                            action: container.removeAction
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        Menu {
                            id: menu
                            contentData: [container.moveUpAction, container.moveDownAction, container.removeAction]
                        }
                        onClicked: menu.popup()
                    }
                    HoverHandler {
                        id: hoverHandler
                    }
                    DescriptiveText.activated: hoverHandler.hovered && (layout.item?.height ?? 0) < 12
                    DescriptiveText.toolTip: d.additionalTrackLoader?.componentName(layout.modelData) ?? ""
                }
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: Theme.paneSeparatorColor
                }
            }
        }
    }
}