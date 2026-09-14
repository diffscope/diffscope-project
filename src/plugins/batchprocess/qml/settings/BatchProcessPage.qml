// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false
    property string scriptDirectory: ""
    property string scriptDataDirectory: ""
    property int maximumConsoleMessageCount: 4096

    onScriptDirectoryChanged: if (started) pageHandle.markDirty()
    onScriptDataDirectoryChanged: if (started) pageHandle.markDirty()
    onMaximumConsoleMessageCountChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    contentWidth: availableWidth

    readonly property TextMatcher matcher: TextMatcher {}

    FolderDialog {
        id: scriptDirectoryDialog

        title: qsTr("Select Script Directory")
        onAccepted: page.scriptDirectory = page.pageHandle.localFilePath(selectedFolder)
    }

    FolderDialog {
        id: scriptDataDirectoryDialog

        title: qsTr("Select Script Data Directory")
        onAccepted: page.scriptDataDirectory = page.pageHandle.localFilePath(selectedFolder)
    }

    ColumnLayout {
        width: page.width

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32

            GroupBox {
                title: qsTr("Batch Process")
                Layout.fillWidth: true
                TextMatcherItem on title { matcher: page.matcher }

                GridLayout {
                    anchors.fill: parent
                    columns: 4

                    Label {
                        text: qsTr("Script directory")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    TextField {
                        Layout.fillWidth: true
                        readOnly: true
                        text: page.scriptDirectory
                    }
                    ToolButton {
                        text: qsTr("Restore Default Directory")
                        icon.source: "image://fluent-system-icons/arrow_reset"
                        display: AbstractButton.IconOnly
                        onClicked: page.scriptDirectory = page.pageHandle.defaultScriptDirectory()
                    }
                    ToolButton {
                        text: qsTr("Browse")
                        icon.source: "image://fluent-system-icons/folder_open"
                        display: AbstractButton.IconOnly
                        onClicked: scriptDirectoryDialog.open()
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.columnSpan: 4
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        text: qsTr("Scripts may perform harmful actions. Make sure you trust a script before adding it.")
                    }
                }
            }

            GroupBox {
                title: qsTr("File System Access")
                Layout.fillWidth: true
                TextMatcherItem on title { matcher: page.matcher }

                GridLayout {
                    anchors.fill: parent
                    columns: 4

                    Label {
                        text: qsTr("Script Data Directory")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    TextField {
                        Layout.fillWidth: true
                        readOnly: true
                        text: page.scriptDataDirectory
                    }
                    ToolButton {
                        text: qsTr("Restore Default Directory")
                        icon.source: "image://fluent-system-icons/arrow_reset"
                        display: AbstractButton.IconOnly
                        onClicked: page.scriptDataDirectory = page.pageHandle.defaultScriptDataDirectory()
                    }
                    ToolButton {
                        text: qsTr("Browse")
                        icon.source: "image://fluent-system-icons/folder_open"
                        display: AbstractButton.IconOnly
                        onClicked: scriptDataDirectoryDialog.open()
                    }
                }
            }

            GroupBox {
                title: qsTr("JavaScript Console")
                Layout.fillWidth: true
                TextMatcherItem on title { matcher: page.matcher }

                GridLayout {
                    anchors.fill: parent
                    columns: 2

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Maximum Retained Messages")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    SpinBox {
                        from: 1
                        to: 1048576
                        value: page.maximumConsoleMessageCount
                        onValueModified: page.maximumConsoleMessageCount = value
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
