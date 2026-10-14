// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Item {
    id: page
    required property QtObject pageHandle
    property bool started: false
    readonly property TextMatcher matcher: TextMatcher {}
    anchors.fill: parent

    function editShortcut() {
        pageHandle.editShortcut(page.Window.window)
    }
    function confirmRestore() {
        if (MessageBox.question(qsTr("Restore Shortcuts"), qsTr("Restore all shortcuts in this preset to their defaults?"),
                                SVSCraft.Yes | SVSCraft.No, SVSCraft.No) === SVSCraft.Yes)
            pageHandle.restoreAll()
    }

    Action {
        id: editShortcutAction
        text: qsTr("Edit Shortcut...")
        enabled: page.pageHandle.canEdit
        onTriggered: page.editShortcut()
    }
    Action {
        id: clearShortcutAction
        text: qsTr("Clear Shortcut")
        enabled: page.pageHandle.canEdit && page.pageHandle.currentShortcut.length > 0
        onTriggered: page.pageHandle.clearShortcut()
    }
    Action {
        id: restoreShortcutAction
        text: qsTr("Restore Shortcut")
        enabled: page.pageHandle.canRestoreAction
        onTriggered: page.pageHandle.restoreAction()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        anchors.topMargin: 0
        spacing: 8
        RowLayout {
            Label {
                text: qsTr("Preset")
                TextMatcherItem on text { matcher: page.matcher }
            }
            ComboBox {
                id: presetCombo
                Layout.fillWidth: true
                model: page.pageHandle.presets
                textRole: "name"
                valueRole: "id"
                Accessible.name: qsTr("Preset")
                currentIndex: {
                    const list = page.pageHandle.presets
                    for (let i = 0; i < list.length; ++i) {
                        if (list[i].id === page.pageHandle.currentPresetId)
                            return i
                    }
                    return 0
                }
                onActivated: index => page.pageHandle.currentPresetId = model[index].id
            }
            Button {
                text: qsTr("Save As...")
                onClicked: nameDialog.show(false)
                TextMatcherItem on text { matcher: page.matcher }
            }
            Button {
                text: qsTr("Rename...")
                enabled: page.pageHandle.currentPresetId.length > 0
                onClicked: nameDialog.show(true)
                TextMatcherItem on text { matcher: page.matcher }
            }
            Button {
                text: qsTr("Delete")
                enabled: page.pageHandle.currentPresetId.length > 0
                onClicked: {
                    if (page.MessageBox.question(qsTr("Delete Preset"), qsTr('Delete preset "%1"?').arg(presetCombo.currentText),
                                                 SVSCraft.Yes | SVSCraft.No, SVSCraft.No) === SVSCraft.Yes)
                        page.pageHandle.deletePreset()
                }
            }
            Button {
                text: qsTr("Restore All")
                enabled: page.pageHandle.currentPresetId.length > 0
                onClicked: page.confirmRestore()
                TextMatcherItem on text { matcher: page.matcher }
            }
        }
        RowLayout {
            Button { text: qsTr("Expand All"); onClicked: catalog.expandRecursively() }
            Button { text: qsTr("Collapse All"); onClicked: catalog.collapseRecursively() }
            TextField {
                id: search
                Layout.fillWidth: true
                placeholderText: qsTr("Search")
                Accessible.name: placeholderText
                ThemedItem.icon.source: "image://fluent-system-icons/search"
                onTextChanged: {
                    page.pageHandle.model.searchText = text
                    if (text.length)
                        Qt.callLater(() => catalog.expandRecursively())
                }
            }
            Button {
                text: qsTr("Find Shortcut...")
                onClicked: {
                    page.pageHandle.findShortcut(page.Window.window)
                    catalog.expandRecursively()
                }
                TextMatcherItem on text { matcher: page.matcher }
            }
        }
        RowLayout {
            visible: page.pageHandle.shortcutFilterText.length > 0
            Label {
                text: qsTr("Shortcut: %1").arg(page.pageHandle.shortcutFilterText)
                Layout.fillWidth: true
            }
            Button { text: qsTr("Clear Filter"); onClicked: page.pageHandle.clearShortcutFilter() }
        }
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ActionCatalogView {
                id: catalog
                anchors.fill: parent
                anchors.margins: 1
                model: page.pageHandle.model
                showShortcuts: true
                onItemSelected: (id, type, command) => page.pageHandle.selectAction(id)
                onItemActivated: (id, type, command) => {
                    page.pageHandle.selectAction(id)
                    if (command)
                        editShortcutAction.trigger()
                }
                onContextMenuRequested: (x, y) => commandMenu.popup(x, y)
                Keys.onDeletePressed: event => {
                    event.accepted = catalog.selectionModel.currentIndex.valid && clearShortcutAction.enabled
                    if (event.accepted)
                        clearShortcutAction.trigger()
                }
            }
        }
        Label {
            Layout.fillWidth: true
            visible: page.pageHandle.errorText.length > 0
            text: page.pageHandle.errorText
            wrapMode: Text.WordWrap
        }
    }

    Menu {
        id: commandMenu
        parent: catalog
        MenuItem { action: editShortcutAction }
        MenuItem { action: clearShortcutAction }
        MenuItem { action: restoreShortcutAction }
    }

    Dialog {
        id: nameDialog
        parent: page.Overlay.overlay
        anchors.centerIn: parent
        width: parent ? Math.min(460, Math.max(0, parent.width - 32)) : 460
        modal: true
        property bool renaming: false
        title: renaming ? qsTr("Rename Preset") : qsTr("Save Preset As")
        function show(rename) {
            renaming = rename
            nameInput.text = rename ? presetCombo.currentText : qsTr("%1 (copy)").arg(presetCombo.currentText)
            open()
        }
        function save() {
            const success = renaming ? page.pageHandle.renamePreset(nameInput.text) : page.pageHandle.saveAs(nameInput.text)
            if (success)
                close()
        }
        onOpened: {
            nameInput.forceActiveFocus()
            nameInput.selectAll()
        }
        contentItem: ColumnLayout {
            spacing: 8
            TextField {
                id: nameInput
                Layout.fillWidth: true
                placeholderText: qsTr("Preset name")
                Accessible.name: placeholderText
                onAccepted: nameDialog.save()
            }
            Label {
                text: page.pageHandle.errorText
                visible: text.length > 0
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Cancel"); onClicked: nameDialog.close() }
                Button {
                    text: qsTr("Save")
                    enabled: nameInput.text.trim().length > 0
                    onClicked: nameDialog.save()
                }
            }
        }
    }
}
