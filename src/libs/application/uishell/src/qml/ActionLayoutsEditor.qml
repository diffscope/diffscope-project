// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl as Impl

import SVSCraft
import SVSCraft.UIComponents

ActionTreeView {
    id: treeView

    property QtObject actionRegistry: null
    property string filterText
    property var typeTextProvider: null
    property var pathProvider: null
    property var expansionKeyProvider: null
    property var expansionIndexProvider: null
    property var savedExpansionKeys: []
    property bool dragEnabled: false
    signal entryDropped(var path, var target, int placement)

    Connections {
        target: treeView.model
        function onModelAboutToBeReset() {
            if (!treeView.expansionKeyProvider)
                return
            const keys = []
            for (let row = 0; row < treeView.rows; ++row) {
                if (treeView.isExpanded(row))
                    keys.push(treeView.expansionKeyProvider(treeView.index(row, 0)))
            }
            treeView.savedExpansionKeys = keys
        }
        function onModelReset() {
            if (!treeView.expansionIndexProvider)
                return
            const keys = treeView.savedExpansionKeys
            Qt.callLater(() => {
                for (const key of keys) {
                    const index = treeView.expansionIndexProvider(key)
                    if (index.valid) {
                        treeView.expandToIndex(index)
                        const row = treeView.rowAtIndex(index)
                        if (row >= 0)
                            treeView.expand(row)
                    }
                }
            })
        }
    }

    ActionLayoutsEditorHelper {
        id: helper
        actionRegistry: treeView.actionRegistry
    }

    delegate: ActionTreeViewDelegate {
        id: control

        actionTreeView: treeView
        readonly property var info: helper.getActionDisplayInfo(model.display, model.entry)
        text: info.text
        icon.source: info.iconSource
        icon.color: info.iconColor.valid ? info.iconColor : primaryTextColor
        property var dragPath: []
        DragHandler {
            id: dragHandler
            target: null
            enabled: treeView.dragEnabled && control.depth > 0
            onActiveChanged: {
                if (active) {
                    control.dragPath = treeView.pathProvider(control.modelIndex)
                    dragProxy.Drag.active = true
                } else {
                    dragProxy.Drag.drop()
                    dragProxy.Drag.active = false
                }
            }
        }
        Item {
            id: dragProxy
            x: dragHandler.centroid.position.x
            y: dragHandler.centroid.position.y
            width: 1
            height: 1
            Drag.source: control
            Drag.keys: ["diffscope-action-layout"]
            Drag.supportedActions: Qt.MoveAction
        }
        DropArea {
            id: dropArea
            anchors.fill: parent
            keys: ["diffscope-action-layout"]
            property int placement: 0
            onPositionChanged: drag => placement = drag.y < height / 4 ? -1 : drag.y > height * 3 / 4 ? 1 : 0
            onDropped: drop => {
                const path = drop.source.dragPath
                const target = control.modelIndex
                const position = placement
                drop.accept(Qt.MoveAction)
                Qt.callLater(() => treeView.entryDropped(path, target, position))
            }
        }
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            y: dropArea.placement < 0 ? 0 : parent.height - height
            height: 2
            color: Theme.accentColor
            visible: dropArea.containsDrag && dropArea.placement !== 0
        }

        contentItem: RowLayout {
            spacing: control.spacing
            Impl.IconLabel {
                id: iconLabel
                icon: control.icon
                color: label.color
                visible: width !== 0 && control.info.type !== ActionLayoutsEditorHelper.Separator && control.info.type !== ActionLayoutsEditorHelper.Stretch
            }
            Impl.IconLabel {
                id: separatorIcon
                visible: control.info.type === ActionLayoutsEditorHelper.Separator || control.info.type === ActionLayoutsEditorHelper.Stretch
                icon.source: control.info.type === ActionLayoutsEditorHelper.Separator ? "image://fluent-system-icons/line_horizontal_1_dashes" : "image://fluent-system-icons/auto_fit_width"
                icon.color: typeLabel.color
                icon.width: 16
                icon.height: 16
            }
            Text {
                id: label
                text: helper.highlightString(control.text, treeView.filterText, Theme.highlightColor)
                font: control.font
                color: control.primaryTextColor
                textFormat: Text.RichText
                visible: control.info.type !== ActionLayoutsEditorHelper.Separator && control.info.type !== ActionLayoutsEditorHelper.Stretch
            }
            Text {
                id: typeLabel
                text: {
                    if (control.depth === 0) {
                        return ""
                    }
                    if (treeView.typeTextProvider)
                        return treeView.typeTextProvider(control.info.id, control.info.type)
                    if (control.info.type === ActionLayoutsEditorHelper.Action) {
                        return qsTr("Action")
                    }
                    if (control.info.type === ActionLayoutsEditorHelper.Menu) {
                        return qsTr("Menu")
                    }
                    if (control.info.type === ActionLayoutsEditorHelper.Group) {
                        return qsTr("Action group")
                    }
                    if (control.info.type === ActionLayoutsEditorHelper.Separator) {
                        return qsTr("Separator")
                    }
                    if (control.info.type === ActionLayoutsEditorHelper.Stretch) {
                        return qsTr("Stretch")
                    }
                }
                font: control.font
                color: control.secondaryTextColor
                Layout.fillWidth: true
                leftPadding: control.info.type === ActionLayoutsEditorHelper.Separator || control.info.type === ActionLayoutsEditorHelper.Stretch ? 0 : 8
            }
        }
    }
}
