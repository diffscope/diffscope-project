// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.Synth

Item {
    id: page

    required property QtObject pageHandle
    required property var configurationModel
    readonly property TextMatcher matcher: TextMatcher {}
    property int editorRevision: 0
    readonly property var currentEditor: {
        editorRevision
        return architectureEditors.itemAt(architectureList.currentIndex)
    }

    anchors.fill: parent

    function savePanels(): bool {
        for (let i = 0; i < architectureEditors.count; ++i) {
            const editor = architectureEditors.itemAt(i)
            if (editor && !editor.savePanel()) {
                architectureList.currentIndex = i
                return false
            }
        }
        return true
    }

    FileDialog {
        id: panelDialog
        property var targetEditor: null
        title: qsTr("Select Control Panel")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("QML files (*.qml)")]
        onAccepted: {
            if (targetEditor)
                targetEditor.selectProvider(selectedFile)
            targetEditor = null
        }
        onRejected: targetEditor = null
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: visible ? 8 : 0
            visible: text.length > 0
            text: page.pageHandle.errorMessage
            color: Theme.errorColor
            wrapMode: Text.Wrap
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12

            Frame {
                SplitView.preferredWidth: 220
                SplitView.minimumWidth: 180
                SplitView.maximumWidth: 400
                padding: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    ListView {
                        id: architectureList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        focus: true
                        model: page.configurationModel
                        currentIndex: -1

                        onCountChanged: {
                            if (count === 0)
                                currentIndex = -1
                            else if (currentIndex < 0)
                                currentIndex = 0
                            else if (currentIndex >= count)
                                currentIndex = count - 1
                        }

                        delegate: ItemDelegate {
                            id: architectureDelegate
                            required property var model
                            required property int index

                            width: ListView.view.width
                            padding: 4
                            leftPadding: 8
                            rightPadding: 8
                            highlighted: ListView.isCurrentItem
                            text: model.architectureId.length > 0 ? model.architectureId : qsTr("Unnamed architecture")
                            ThemedItem.flat: true
                            ThemedItem.controlType: highlighted ? SVS.CT_Accent : SVS.CT_Normal
                            background: ButtonRectangle {
                                control: architectureDelegate
                                checked: architectureDelegate.highlighted
                                flat: true
                            }
                            onClicked: architectureList.currentIndex = index
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: architectureList.count === 0
                            text: qsTr("No architecture configurations")
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.borderColor
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 4
                        spacing: 2

                        ToolButton {
                            text: qsTr("Add Architecture Configuration")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/add"
                            onClicked: architectureList.currentIndex = page.configurationModel.addEntry()
                        }
                        ToolButton {
                            text: qsTr("Delete")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/delete"
                            enabled: Boolean(page.currentEditor) && !page.currentEditor.builtIn
                            onClicked: {
                                const oldIndex = architectureList.currentIndex
                                if (page.configurationModel.removeEntry(oldIndex))
                                    architectureList.currentIndex = Math.min(oldIndex, architectureList.count - 1)
                            }
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }

            Pane {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                padding: 0
                leftPadding: mirrored ? 0 : 12
                rightPadding: mirrored ? 12 : 0
                background: null

                Repeater {
                    id: architectureEditors
                    model: page.configurationModel
                    onItemAdded: page.editorRevision++
                    onItemRemoved: page.editorRevision++

                    delegate: ColumnLayout {
                        id: editor
                        required property var model
                        required property int index
                        readonly property bool builtIn: model.builtIn
                        readonly property var configuredExtra: model.extra
                        property bool complete: false

                        anchors.fill: parent
                        visible: index === architectureList.currentIndex
                        spacing: 12
                        onConfiguredExtraChanged: {
                            if (complete)
                                extraEditor.loadState(configuredExtra)
                        }
                        Component.onCompleted: {
                            complete = true
                            extraEditor.loadState(configuredExtra)
                        }

                        function savePanel(): bool {
                            if (!extraEditor.captureState())
                                return false
                            model.extra = extraEditor.saveState()
                            return true
                        }

                        function selectProvider(url) {
                            savePanel()
                            model.providerUrl = url
                        }

                        ControlPanelProvider {
                            id: provider
                            architectureId: editor.model.architectureId.trim()
                            preview: true
                            source: editor.model.providerUrl
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 8

                            Label {
                                text: qsTr("Architecture ID")
                                TextMatcherItem on text { matcher: page.matcher }
                            }
                            TextField {
                                Layout.fillWidth: true
                                text: editor.model.architectureId
                                readOnly: editor.builtIn
                                Accessible.name: qsTr("Architecture ID")
                                onTextEdited: editor.model.architectureId = text
                            }

                            Label {
                                text: qsTr("Control Panel")
                                TextMatcherItem on text { matcher: page.matcher }
                            }
                            RowLayout {
                                Layout.fillWidth: true

                                TextField {
                                    Layout.fillWidth: true
                                    readOnly: true
                                    text: decodeURIComponent(editor.model.providerUrl.toString().split("/").pop())
                                    placeholderText: editor.builtIn ? qsTr("Default") : qsTr("None")
                                    Accessible.name: qsTr("Control Panel")
                                }
                                ToolButton {
                                    text: qsTr("Browse...")
                                    display: AbstractButton.IconOnly
                                    icon.source: "image://fluent-system-icons/folder_open"
                                    onClicked: {
                                        panelDialog.targetEditor = editor
                                        panelDialog.open()
                                    }
                                }
                                ToolButton {
                                    text: qsTr("Remove Control Panel")
                                    display: AbstractButton.IconOnly
                                    icon.source: "image://fluent-system-icons/dismiss"
                                    enabled: editor.model.providerUrl.toString().length > 0
                                    onClicked: editor.selectProvider("")
                                }
                            }
                        }

                        ExtraControlPanel {
                            id: extraEditor
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            padding: 0
                            panelComponent: provider.architectureControlPanelComponent
                            providerLoading: provider.loading
                            providerFailed: provider.failed
                            onStateEdited: editor.model.extra = saveState()
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: architectureList.currentIndex < 0
                    text: qsTr("Select an architecture configuration to edit.")
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                }
            }
        }
    }
}
