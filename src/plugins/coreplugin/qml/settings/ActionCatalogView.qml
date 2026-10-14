// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.impl as Impl

import SVSCraft

import DiffScope.UIShell as UIShell

UIShell.ActionTreeView {
    id: tree

    signal itemSelected(string actionId, int entryType, bool command)
    signal itemActivated(string actionId, int entryType, bool command)
    property bool showShortcuts: false
    property var expandedIds: []
    property string currentId: ""
    property int modelRevision: 0

    onIndexSelected: index => {
        const info = model.itemInfo(index)
        itemSelected(info.id, info.entryType, info.command)
    }
    onIndexActivated: index => {
        const info = model.itemInfo(index)
        itemActivated(info.id, info.entryType, info.command)
    }

    Connections {
        target: tree.model
        function onModelAboutToBeReset() {
            ++tree.modelRevision
            const ids = []
            for (let row = 0; row < tree.rows; ++row) {
                if (tree.isExpanded(row))
                    ids.push(tree.model.itemInfo(tree.index(row, 0)).id)
            }
            tree.expandedIds = ids
            const index = tree.selectionModel.currentIndex
            tree.currentId = index.valid ? tree.model.itemInfo(index).id : ""
        }
        function onModelReset() {
            const ids = tree.expandedIds
            const selectedId = tree.currentId
            const revision = tree.modelRevision
            Qt.callLater(() => {
                if (revision !== tree.modelRevision)
                    return
                for (const id of ids) {
                    const index = tree.model.indexForId(id)
                    if (index.valid) {
                        tree.expandToIndex(index)
                        const row = tree.rowAtIndex(index)
                        if (row >= 0)
                            tree.expand(row)
                    }
                }
                const index = tree.model.indexForId(selectedId)
                if (index.valid)
                    tree.selectIndex(index)
            })
        }
    }

    delegate: UIShell.ActionTreeViewDelegate {
        id: control

        actionTreeView: tree
        text: model.title
        icon.source: model.iconSource
        icon.color: model.iconColor.valid ? model.iconColor : primaryTextColor

        contentItem: RowLayout {
            spacing: control.spacing
            Impl.IconLabel {
                id: iconLabel
                icon: control.icon
                color: control.primaryTextColor
                visible: implicitWidth > 0
            }
            Text {
                text: control.text
                textFormat: Text.PlainText
                font: control.font
                color: control.primaryTextColor
                elide: Text.ElideRight
                Layout.fillWidth: tree.showShortcuts
                Layout.maximumWidth: Math.max(0, control.availableWidth - iconLabel.implicitWidth - Math.min(tree.showShortcuts ? (shortcutBadge.visible ? shortcutBadge.implicitWidth : 0) : typeLabel.implicitWidth, tree.width / 2) - (customLabel.visible ? Math.min(customLabel.implicitWidth, tree.width / 4) : 0) - control.spacing * 3)
            }
            Text {
                id: typeLabel
                visible: !tree.showShortcuts
                text: model.typeText
                textFormat: Text.PlainText
                font: control.font
                color: control.secondaryTextColor
                elide: Text.ElideRight
                horizontalAlignment: control.mirrored ? Text.AlignRight : Text.AlignLeft
                leftPadding: text.length > 0 ? 8 : 0
                Layout.fillWidth: true
                Layout.maximumWidth: tree.width / 2
            }
            UIShell.ShortcutBadge {
                id: shortcutBadge
                visible: tree.showShortcuts && text.length > 0
                text: model.shortcut
                font: control.font
                textColor: control.primaryTextColor
                Layout.alignment: Qt.AlignVCenter
                Layout.maximumWidth: tree.width / 2
            }
            Text {
                id: customLabel
                visible: tree.showShortcuts && model.customized
                text: qsTr("Custom")
                textFormat: Text.PlainText
                font: control.font
                color: control.secondaryTextColor
                elide: Text.ElideRight
                Layout.maximumWidth: tree.width / 4
            }
        }
    }
}
