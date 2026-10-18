// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

ScrollView {
    id: page

    required property QtObject pageHandle
    readonly property var configuration: pageHandle.configuration
    readonly property var selectedDevice: pageHandle.selectedDevice
    readonly property QtObject deviceCatalog: pageHandle.deviceCatalog
    readonly property TextMatcher matcher: TextMatcher {}

    anchors.fill: parent
    contentWidth: availableWidth

    FileDialog {
        id: executableDialog
        title: qsTr("Add DiffSinger Synthesis Program Installation")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Executable files (*)")]
        onAccepted: page.pageHandle.addLocalInstallation(selectedFile)
    }

    FolderDialog {
        id: packageDialog
        title: qsTr("Select Package Directory")
        onAccepted: page.pageHandle.setValue("packageDirectory", page.pageHandle.localFilePath(selectedFolder))
    }

    ColumnLayout {
        width: page.availableWidth
        spacing: 24

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 24

            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: page.pageHandle.errorMessage
                textFormat: Text.PlainText
                color: Theme.errorColor
                wrapMode: Text.Wrap
            }

            GroupBox {
                title: qsTr("DiffSinger Synthesis Program")
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
                            currentValue: page.configuration.installationId
                            Accessible.name: installationLabel.text
                            onActivated: page.pageHandle.setValue("installationId", currentValue)
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
                                        color: Theme.foregroundSecondaryColor
                                        font.pixelSize: 11
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
                            onClicked: executableDialog.open()
                        }

                        ToolButton {
                            text: qsTr("Remove Local Installation from List")
                            icon.source: "image://fluent-system-icons/subtract"
                            display: AbstractButton.IconOnly
                            focusPolicy: Qt.StrongFocus
                            enabled: page.configuration.installationId.startsWith("local:")
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
                        text: page.configuration.executablePath
                        placeholderText: qsTr("No DiffSinger Synthesis Program installation is selected")
                        Accessible.name: qsTr("Executable path")
                        TextMatcherItem on placeholderText { matcher: page.matcher }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        Button {
                            text: qsTr("Download DiffSinger Synthesis Program...")
                            icon.source: "image://fluent-system-icons/arrow_download"
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        Button {
                            text: qsTr("Uninstall Downloaded Version")
                            icon.source: "image://fluent-system-icons/delete"
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                    }

                    CheckBox {
                        text: qsTr("Automatically start DiffSinger Synthesis Program")
                        checked: page.configuration.autoStart
                        TextMatcherItem on text { matcher: page.matcher }
                        onClicked: page.pageHandle.setValue("autoStart", checked)
                    }
                }
            }

            GroupBox {
                title: qsTr("Server Options")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                GridLayout {
                    anchors.fill: parent
                    columns: 3

                    Label {
                        id: hostLabel
                        text: qsTr("Host")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    TextField {
                        Layout.fillWidth: true
                        Layout.columnSpan: 2
                        text: page.configuration.host
                        Accessible.name: hostLabel.text
                        onTextEdited: page.pageHandle.setValue("host", text)
                    }

                    Label {
                        id: portLabel
                        text: qsTr("Port")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    SpinBox {
                        Layout.columnSpan: 2
                        from: 1
                        to: 65535
                        editable: true
                        value: page.configuration.port
                        Accessible.name: portLabel.text
                        onValueModified: page.pageHandle.setValue("port", value)
                    }

                    Label {
                        id: packageLabel
                        text: qsTr("Package directory")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: page.configuration.packageDirectory
                        placeholderText: qsTr("Default package directory")
                        Accessible.name: packageLabel.text
                        onTextEdited: page.pageHandle.setValue("packageDirectory", text)
                        onEditingFinished: page.pageHandle.normalizePackageDirectory()
                    }
                    ToolButton {
                        text: qsTr("Browse for Package Directory...")
                        icon.source: "image://fluent-system-icons/folder_open"
                        display: AbstractButton.IconOnly
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: text
                        ToolTip.visible: hovered
                        ToolTip.text: text
                        onClicked: packageDialog.open()
                    }

                    Label {
                        id: logLevelLabel
                        text: qsTr("Log level")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.columnSpan: 2
                        model: [
                            { id: "trace", name: qsTr("Trace") },
                            { id: "debug", name: qsTr("Debug") },
                            { id: "success", name: qsTr("Success") },
                            { id: "info", name: qsTr("Information") },
                            { id: "warning", name: qsTr("Warning") },
                            { id: "critical", name: qsTr("Critical") },
                            { id: "fatal", name: qsTr("Fatal") }
                        ]
                        textRole: "name"
                        valueRole: "id"
                        currentValue: page.configuration.logLevel
                        Accessible.name: logLevelLabel.text
                        onActivated: page.pageHandle.setValue("logLevel", currentValue)
                    }
                }
            }

            GroupBox {
                title: qsTr("Logging")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2

                        Label {
                            id: logFileSizeLabel
                            text: qsTr("Max file size")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        RowLayout {
                            SpinBox {
                                from: 1
                                to: 2147483647
                                editable: true
                                value: Math.round(page.configuration.logMaxFileSize / 1024)
                                Accessible.name: logFileSizeLabel.text
                                onValueModified: page.pageHandle.setValue("logMaxFileSize", value * 1024)
                            }
                            Label { text: qsTr("KiB") }
                        }

                        Label {
                            id: logArchiveSizeLabel
                            text: qsTr("Max archive size")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        RowLayout {
                            SpinBox {
                                from: 1
                                to: 2147483647
                                editable: true
                                value: Math.round(page.configuration.logMaxArchiveSize / 1024)
                                Accessible.name: logArchiveSizeLabel.text
                                onValueModified: page.pageHandle.setValue("logMaxArchiveSize", value * 1024)
                            }
                            Label { text: qsTr("KiB") }
                        }

                        Label {
                            id: logArchiveDaysLabel
                            text: qsTr("Max archive days")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        SpinBox {
                            from: 1
                            to: 3650
                            editable: true
                            value: page.configuration.logMaxArchiveDays
                            Accessible.name: logArchiveDaysLabel.text
                            onValueModified: page.pageHandle.setValue("logMaxArchiveDays", value)
                        }

                        Label {
                            id: logCompressionLabel
                            text: qsTr("Compression level")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        RowLayout {
                            SpinBox {
                                from: 0
                                to: 9
                                editable: true
                                value: page.configuration.logCompressLevel
                                Accessible.name: logCompressionLabel.text
                                onValueModified: page.pageHandle.setValue("logCompressLevel", value)
                            }
                            Label {
                                text: qsTr("%L1 = no compression, %L2 = best compression").arg(0).arg(9)
                                color: Theme.foregroundSecondaryColor
                            }
                        }
                    }

                    CheckBox {
                        text: qsTr("Notify about warnings")
                        checked: page.configuration.notifyWarnings
                        TextMatcherItem on text { matcher: page.matcher }
                        onClicked: page.pageHandle.setValue("notifyWarnings", checked)
                    }

                    TextField {
                        Layout.fillWidth: true
                        readOnly: true
                        selectByMouse: true
                        text: page.pageHandle.logsLocation
                        Accessible.name: qsTr("Log directory")
                    }
                    Button {
                        text: qsTr("Open in %1").arg(DesktopServices.fileManagerName)
                        TextMatcherItem on text { matcher: page.matcher }
                        onClicked: DesktopServices.reveal(page.pageHandle.logsLocation)
                    }
                }
            }

            GroupBox {
                title: qsTr("Execution Device")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent

                    CheckBox {
                        text: qsTr("Use the default execution provider and device")
                        checked: page.configuration.useDefaultDevice
                        TextMatcherItem on text { matcher: page.matcher }
                        onClicked: page.pageHandle.setValue("useDefaultDevice", checked)
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2

                        Label {
                            id: providerLabel
                            text: qsTr("Execution provider")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            id: providerCombo
                            Layout.fillWidth: true
                            model: page.deviceCatalog.providers
                            textRole: "name"
                            valueRole: "id"
                            currentValue: page.selectedDevice.executionProvider ?? ""
                            enabled: !page.configuration.useDefaultDevice && !page.deviceCatalog.busy && count > 0
                            Accessible.name: providerLabel.text
                            onActivated: page.pageHandle.setValue("executionProvider", currentValue)
                        }

                        Label {
                            id: deviceLabel
                            text: qsTr("Device")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: page.deviceCatalog.devicesByProvider[page.selectedDevice.executionProvider ?? ""] ?? []
                            textRole: "description"
                            valueRole: "id"
                            currentValue: page.selectedDevice.id ?? ""
                            enabled: !page.configuration.useDefaultDevice && !page.deviceCatalog.busy && count > 0
                            Accessible.name: deviceLabel.text
                            onActivated: page.pageHandle.setValue("deviceId", currentValue)
                        }
                    }

                    RowLayout {
                        Button {
                            text: qsTr("Refresh Devices")
                            enabled: page.configuration.executablePath.length > 0 && !page.deviceCatalog.busy
                            TextMatcherItem on text { matcher: page.matcher }
                            onClicked: page.pageHandle.refreshDevices()
                        }
                        BusyIndicator {
                            running: page.deviceCatalog.busy
                            visible: running
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        text: page.deviceCatalog.errorMessage
                        textFormat: Text.PlainText
                        color: Theme.errorColor
                        wrapMode: Text.Wrap
                    }
                }
            }

            GroupBox {
                title: qsTr("Additional Command-Line Arguments")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                ScrollView {
                    anchors.fill: parent
                    implicitWidth: additionalArgumentsInput.implicitWidth + leftPadding + rightPadding
                    implicitHeight: 120
                    contentWidth: availableWidth
                    TextArea {
                        id: additionalArgumentsInput
                        text: page.configuration.extraArguments
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        Accessible.name: qsTr("Additional command-line arguments")
                        onTextChanged: {
                            if (text !== page.configuration.extraArguments)
                                page.pageHandle.setValue("extraArguments", text)
                        }
                    }
                }
            }
        }
    }
}
