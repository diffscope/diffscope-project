// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell as UIShell
import DiffScope.Core

Item {
    id: page
    required property QtObject pageHandle
    required property QtObject model
    property bool started: false
    readonly property TextMatcher matcher: TextMatcher {}
    readonly property bool containerSelected: pageHandle.currentForm === UIShell.ActionLayoutsEditorHelper.Menu
                                               || pageHandle.currentForm === UIShell.ActionLayoutsEditorHelper.Group
    readonly property bool spacingSelected: pageHandle.currentForm === UIShell.ActionLayoutsEditorHelper.Separator
                                             || pageHandle.currentForm === UIShell.ActionLayoutsEditorHelper.Stretch
    anchors.fill: parent
    function confirmRestore(all) {
        const text = all
            ? qsTr("Restore all menu and toolbar layouts to their defaults?")
            : qsTr("Restore the selected item to its defaults?")
        if (MessageBox.question(qsTr("Restore Layout"), text, SVSCraft.Yes | SVSCraft.No, SVSCraft.No) === SVSCraft.Yes) {
            if (all)
                pageHandle.restoreAll()
            else
                pageHandle.restoreCurrent()
        }
    }
    function synchronizeSelection() {
        if (started)
            editor.selectIndex(pageHandle.currentIndex)
    }
    Connections {
        target: page.pageHandle
        function onStateChanged() {
            Qt.callLater(page.synchronizeSelection)
        }
    }

    Action {
        id: addItemAction
        text: qsTr("Add Action...")
        enabled: page.pageHandle.canAdd
        onTriggered: picker.open()
    }
    Action {
        id: separatorAction
        text: qsTr("Add Separator")
        icon.source: "image://fluent-system-icons/line_horizontal_1_dashes"
        enabled: page.pageHandle.canAdd
        onTriggered: page.pageHandle.addEntry("", UIShell.ActionLayoutsEditorHelper.Separator)
    }
    Action {
        id: stretchAction
        text: qsTr("Add Stretch")
        icon.source: "image://fluent-system-icons/auto_fit_width"
        enabled: page.pageHandle.canAdd
        onTriggered: page.pageHandle.addEntry("", UIShell.ActionLayoutsEditorHelper.Stretch)
    }
    Action {
        id: editIconAction
        text: qsTr("Edit Icon...")
        icon.source: "image://fluent-system-icons/edit"
        enabled: page.pageHandle.canRemove
    }
    Action {
        id: upAction
        text: qsTr("Move Up")
        icon.source: "image://fluent-system-icons/arrow_circle_up"
        enabled: page.pageHandle.canMoveUp
        onTriggered: page.pageHandle.moveCurrent(-1)
    }
    Action {
        id: downAction
        text: qsTr("Move Down")
        icon.source: "image://fluent-system-icons/arrow_circle_down"
        enabled: page.pageHandle.canMoveDown
        onTriggered: page.pageHandle.moveCurrent(1)
    }
    Action {
        id: removeAction
        text: qsTr("Remove")
        icon.source: "image://fluent-system-icons/delete"
        enabled: page.pageHandle.canRemove
        onTriggered: page.pageHandle.removeEntry()
    }
    ActionGroup {
        id: formActionGroup
    }
    Action {
        id: menuFormAction
        text: qsTr("Menu")
        ActionGroup.group: formActionGroup
        checkable: true
        checked: page.pageHandle.currentForm === UIShell.ActionLayoutsEditorHelper.Menu
        enabled: page.pageHandle.canChangeForm && page.containerSelected
        onTriggered: page.pageHandle.changeForm(UIShell.ActionLayoutsEditorHelper.Menu)
    }
    Action {
        id: groupFormAction
        text: qsTr("Action Group")
        ActionGroup.group: formActionGroup
        checkable: true
        checked: page.pageHandle.currentForm === UIShell.ActionLayoutsEditorHelper.Group
        enabled: page.pageHandle.canChangeForm && page.containerSelected
        onTriggered: page.pageHandle.changeForm(UIShell.ActionLayoutsEditorHelper.Group)
    }
    Action {
        id: separatorFormAction
        text: qsTr("Separator")
        icon.source: "image://fluent-system-icons/line_horizontal_1_dashes"
        ActionGroup.group: formActionGroup
        checkable: true
        checked: page.pageHandle.currentForm === UIShell.ActionLayoutsEditorHelper.Separator
        enabled: page.pageHandle.canChangeForm && page.spacingSelected
        onTriggered: page.pageHandle.changeForm(UIShell.ActionLayoutsEditorHelper.Separator)
    }
    Action {
        id: stretchFormAction
        text: qsTr("Stretch")
        icon.source: "image://fluent-system-icons/auto_fit_width"
        ActionGroup.group: formActionGroup
        checkable: true
        checked: page.pageHandle.currentForm === UIShell.ActionLayoutsEditorHelper.Stretch
        enabled: page.pageHandle.canChangeForm && page.spacingSelected
        onTriggered: page.pageHandle.changeForm(UIShell.ActionLayoutsEditorHelper.Stretch)
    }
    Action {
        id: restoreCurrentAction
        text: qsTr("Restore")
        enabled: page.pageHandle.canRestoreCurrent
        onTriggered: page.confirmRestore(false)
    }
    Action {
        id: restoreAllAction
        text: qsTr("Restore All")
        onTriggered: page.confirmRestore(true)
    }
    Shortcut {
        sequence: "Alt+Up"
        enabled: editor.activeFocus && upAction.enabled
        onActivated: upAction.trigger()
    }
    Shortcut {
        sequence: "Alt+Down"
        enabled: editor.activeFocus && downAction.enabled
        onActivated: downAction.trigger()
    }
    Shortcut {
        sequence: "Delete"
        enabled: editor.activeFocus && removeAction.enabled
        onActivated: removeAction.trigger()
    }
    Menu {
        id: addItemsMenu
        title: qsTr("Add")
        icon.source: "image://fluent-system-icons/add_circle"
        MenuItem { action: addItemAction }
        MenuItem { action: separatorAction }
        MenuItem { action: stretchAction }
    }
    Menu {
        id: restoreMenu
        title: qsTr("Restore")
        icon.source: "image://fluent-system-icons/arrow_hook_up_left"
        MenuItem { action: restoreCurrentAction }
        MenuItem { action: restoreAllAction }
    }
    Component {
        id: formSeparatorComponent
        MenuSeparator {}
    }
    Component {
        id: formItemComponent
        MenuItem {}
    }
    Menu {
        id: contextMenu
        parent: editor
        property var activeFormItems: []
        onAboutToShow: {
            for (const item of activeFormItems) {
                removeItem(item)
                item.destroy()
            }
            activeFormItems = []
            if (page.pageHandle.canChangeForm) {
                const actions = page.containerSelected
                    ? [menuFormAction, groupFormAction]
                    : [separatorFormAction, stretchFormAction]
                const position = count - 2
                activeFormItems = [formSeparatorComponent.createObject(contextMenu.contentItem)]
                for (const action of actions)
                    activeFormItems.push(formItemComponent.createObject(contextMenu.contentItem, {action}))
                for (let index = 0; index < activeFormItems.length; ++index)
                    insertItem(position + index, activeFormItems[index])
            }
        }
        MenuItem { action: addItemAction }
        MenuItem { action: separatorAction }
        MenuItem { action: stretchAction }
        MenuSeparator {}
        MenuItem { action: editIconAction }
        MenuItem { action: upAction }
        MenuItem { action: downAction }
        MenuItem { action: removeAction }
        MenuSeparator {}
        MenuItem { action: restoreCurrentAction }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        anchors.topMargin: 0
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            ToolBarContainer {
                Component.onCompleted: {
                    addMenu(addItemsMenu)
                    addAction(editIconAction)
                    addAction(upAction)
                    addAction(downAction)
                    addAction(removeAction)
                    addMenu(restoreMenu)
                }
            }
            TextField {
                id: search
                Layout.fillWidth: true
                placeholderText: qsTr("Search")
                Accessible.name: placeholderText
                ThemedItem.icon.source: "image://fluent-system-icons/search"
                onTextChanged: {
                    page.pageHandle.setSearchText(text)
                    if (text.length)
                        Qt.callLater(() => editor.expandRecursively())
                }
            }
        }
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            UIShell.ActionLayoutsEditor {
                id: editor
                anchors.fill: parent
                anchors.margins: 1
                actionRegistry: CoreInterface.actionRegistry
                model: page.model
                filterText: search.text
                typeTextProvider: (id, type) => page.pageHandle.typeText(id, type)
                pathProvider: index => page.pageHandle.pathForIndex(index)
                expansionKeyProvider: index => page.pageHandle.expansionKey(index)
                expansionIndexProvider: key => page.pageHandle.indexForExpansionKey(key)
                dragEnabled: true
                onIndexSelected: index => page.pageHandle.selectIndex(index)
                onEntryDropped: (path, target, placement) => page.pageHandle.dropEntry(path, target, placement)
                onContextMenuRequested: (x, y) => contextMenu.popup(x, y)
            }
        }
        Label {
            Layout.fillWidth: true
            visible: page.pageHandle.errorText.length > 0
            text: page.pageHandle.errorText
            wrapMode: Text.WordWrap
        }
    }

    Dialog {
        id: picker
        parent: page.Overlay.overlay
        anchors.centerIn: parent
        width: parent ? Math.min(640, Math.max(0, parent.width - 32)) : 640
        height: parent ? Math.min(560, Math.max(0, parent.height - 32)) : 560
        title: qsTr("Add Action, Menu, or Group")
        modal: true
        property string selectedId: ""
        property int selectedType: -1
        function addSelected() {
            if (selectedType >= 0 && page.pageHandle.addEntry(selectedId, selectedType))
                close()
        }
        onOpened: {
            selectedId = ""
            selectedType = -1
            pickerSearch.forceActiveFocus()
        }
        contentItem: ColumnLayout {
            spacing: 8
            TextField {
                id: pickerSearch
                Layout.fillWidth: true
                placeholderText: qsTr("Search")
                Accessible.name: placeholderText
                onTextChanged: {
                    page.pageHandle.catalogModel.searchText = text
                    picker.selectedType = -1
                    if (text.length)
                        Qt.callLater(() => catalog.expandRecursively())
                }
            }
            Frame {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ActionCatalogView {
                    id: catalog
                    anchors.fill: parent
                    anchors.margins: 1
                    model: page.pageHandle.catalogModel
                    onItemSelected: (id, type, command) => {
                        picker.selectedId = id
                        picker.selectedType = type
                    }
                    onItemActivated: (id, type, command) => {
                        picker.selectedId = id
                        picker.selectedType = type
                        picker.addSelected()
                    }
                }
            }
            Label {
                visible: page.pageHandle.errorText.length > 0
                text: page.pageHandle.errorText
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            RowLayout {
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Cancel"); onClicked: picker.close() }
                Button {
                    text: qsTr("Add")
                    enabled: picker.selectedType >= 0
                    onClicked: picker.addSelected()
                }
            }
        }
    }
}
