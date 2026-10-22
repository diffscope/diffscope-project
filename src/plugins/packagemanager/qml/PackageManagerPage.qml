// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.PackageManager

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false
    property string packageDir: ""
    property int timeoutSeconds: 30

    onPackageDirChanged: if (started) pageHandle.markDirty()
    onTimeoutSecondsChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    contentWidth: availableWidth

    readonly property TextMatcher matcher: TextMatcher {}

    FileDialog {
        id: dspmFileDialog
        title: qsTr("Add Package Manager Installation")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Executable files (*)")]
        onAccepted: page.pageHandle.addLocalInstallation(selectedFile)
    }

    FolderDialog {
        id: packageDirDialog
        title: qsTr("Select Package Directory")
        onAccepted: page.packageDir = page.pageHandle.localFilePath(selectedFolder)
    }

    ColumnLayout {
        width: page.availableWidth

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32

            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: page.pageHandle.errorMessage
                textFormat: Text.PlainText
                color: Theme.errorColor
                wrapMode: Text.Wrap
            }

            GroupBox {
                title: qsTr("Package Manager")
                enabled: !page.pageHandle.busy
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            id: installationLabel
                            text: qsTr("Installation")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            id: installationCombo
                            Layout.fillWidth: true
                            model: page.pageHandle.installations
                            textRole: "name"
                            valueRole: "id"
                            currentValue: page.pageHandle.installationId
                            Accessible.name: installationLabel.text
                            onActivated: page.pageHandle.installationId = currentValue
                            delegate: ItemDelegate {
                                id: installationDelegate
                                required property int index
                                required property var modelData
                                width: installationCombo.width
                                highlighted: installationCombo.highlightedIndex === index
                                text: modelData.name
                                Accessible.description: modelData.path
                                contentItem: ColumnLayout {
                                    spacing: 2
                                    Label {
                                        Layout.fillWidth: true
                                        text: installationDelegate.modelData.name
                                        textFormat: Text.PlainText
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        visible: text.length > 0
                                        text: installationDelegate.modelData.path
                                        textFormat: Text.PlainText
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                                        elide: Text.ElideMiddle
                                    }
                                }
                            }
                        }
                        ToolButton {
                            text: qsTr("Add Local Installation...")
                            icon.source: "image://fluent-system-icons/add"
                            display: AbstractButton.IconOnly
                            focusPolicy: Qt.StrongFocus
                            Accessible.name: text
                            ToolTip.visible: hovered
                            ToolTip.text: text
                            onClicked: dspmFileDialog.open()
                        }
                        ToolButton {
                            text: qsTr("Remove Local Installation from List")
                            icon.source: "image://fluent-system-icons/subtract"
                            display: AbstractButton.IconOnly
                            focusPolicy: Qt.StrongFocus
                            enabled: page.pageHandle.installationId.startsWith("local:")
                            Accessible.name: text
                            ToolTip.visible: hovered
                            ToolTip.text: text
                            onClicked: page.pageHandle.removeLocalInstallation()
                        }
                    }
                    TextField {
                        Layout.fillWidth: true
                        readOnly: true
                        selectByMouse: true
                        text: page.pageHandle.executablePath
                        placeholderText: qsTr("No package manager installation is selected")
                        Accessible.name: qsTr("Executable path")
                        TextMatcherItem on placeholderText { matcher: page.matcher }
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        Button {
                            text: qsTr("Download Package Manager...")
                            icon.source: "image://fluent-system-icons/arrow_download"
                            TextMatcherItem on text { matcher: page.matcher }
                            onClicked: page.pageHandle.downloadInstallation()
                        }
                        Button {
                            text: qsTr("Uninstall Downloaded Version")
                            icon.source: "image://fluent-system-icons/delete"
                            enabled: page.pageHandle.installationId.startsWith("downloaded:")
                            TextMatcherItem on text { matcher: page.matcher }
                            onClicked: page.pageHandle.uninstallInstallation()
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Package Options")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                GridLayout {
                    anchors.fill: parent
                    columns: 3
                    Label {
                        id: packageDirLabel
                        text: qsTr("Package directory")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: page.packageDir
                        Accessible.name: packageDirLabel.text
                        onTextEdited: page.packageDir = text
                    }
                    ToolButton {
                        text: qsTr("Browse")
                        icon.source: "image://fluent-system-icons/folder_open"
                        display: AbstractButton.IconOnly
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: text
                        ToolTip.visible: hovered
                        ToolTip.text: text
                        onClicked: packageDirDialog.open()
                    }

                    Label {
                        id: timeoutLabel
                        text: qsTr("Output timeout")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    RowLayout {
                        Layout.columnSpan: 2
                        SpinBox {
                            from: 1
                            to: 300
                            value: page.timeoutSeconds
                            editable: true
                            Accessible.name: timeoutLabel.text
                            onValueModified: page.timeoutSeconds = value
                        }
                        Label {
                            text: qsTr("seconds")
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
