// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Window {
    id: dialog

    required property string productName
    required property QtObject catalog
    property string phase: "selection"
    property string statusText: ""
    property bool completed: false
    property var finalResult
    property var escapeButton: SVS.Cancel
    property real progress: 0
    property double bytesReceived: 0
    property double bytesTotal: -1

    signal done(var result)

    function finish(result) {
        if (completed)
            return
        completed = true
        finalResult = result
        done(result)
        close()
    }

    function yieldResult(result) {
        done(result)
    }

    function formatBytes(value) {
        const units = [qsTr("B"), qsTr("KiB"), qsTr("MiB"), qsTr("GiB"), qsTr("TiB")]
        let size = Math.max(0, value)
        let unit = 0
        while (size >= 1024 && unit < units.length - 1) {
            size /= 1024
            ++unit
        }
        return size.toLocaleString(Qt.locale(), "f", unit === 0 ? 0 : 1) + " " + units[unit]
    }

    title: qsTr("Download %1").arg(dialog.productName)
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    width: 560
    minimumWidth: 480
    minimumHeight: layout.implicitHeight
    maximumHeight: layout.implicitHeight
    height: layout.implicitHeight

    onClosing: (event) => {
        if (!completed) {
            completed = true
            finalResult = "cancelled"
            done("cancelled")
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundPrimaryColor
    }

    ColumnLayout {
        id: layout
        width: dialog.width
        spacing: 0

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 16

            Label {
                Layout.fillWidth: true
                text: {
                    switch (dialog.phase) {
                    case "download": return qsTr("Downloading %1...").arg(dialog.productName)
                    case "verify": return qsTr("Verifying the download...")
                    case "install": return qsTr("Installing %1...").arg(dialog.productName)
                    case "validate": return dialog.statusText
                    default: return dialog.catalog.busy ? qsTr("Retrieving available versions...") : qsTr("Select a version to download.")
                    }
                }
                wrapMode: Text.Wrap
                Accessible.role: Accessible.StaticText
            }

            TabBar {
                id: channelTabs
                visible: dialog.phase === "selection"
                Layout.fillWidth: true
                currentIndex: dialog.catalog.channelIndex
                Repeater {
                    model: dialog.catalog.channels
                    TabButton {
                        required property int index
                        required property var modelData
                        text: modelData.name
                        enabled: !dialog.catalog.busy && modelData.available
                        onClicked: dialog.catalog.channelIndex = index
                    }
                }
            }

            RowLayout {
                visible: dialog.phase === "selection" && dialog.catalog.versions.length > 0
                Layout.fillWidth: true
                Label {
                    id: versionLabel
                    text: qsTr("Version")
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: dialog.catalog.versions
                    currentIndex: dialog.catalog.versionIndex
                    enabled: !dialog.catalog.busy
                    Accessible.name: versionLabel.text
                    onActivated: dialog.catalog.versionIndex = currentIndex
                }
            }

            Label {
                Layout.fillWidth: true
                visible: dialog.phase === "selection" && !dialog.catalog.busy && dialog.catalog.errorMessage.length === 0 && !dialog.catalog.available
                text: dialog.catalog.versions.length === 0 ? qsTr("No versions have been released.") : qsTr("This version is not available for this system.")
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                visible: dialog.phase === "selection" && dialog.catalog.errorMessage.length > 0
                text: dialog.catalog.errorMessage
                textFormat: Text.PlainText
                color: Theme.errorColor
                wrapMode: Text.Wrap
            }

            Button {
                visible: dialog.phase === "selection" && dialog.catalog.errorMessage.length > 0
                text: qsTr("Retry")
                onClicked: dialog.catalog.retry()
            }

            Button {
                visible: dialog.phase === "selection" && dialog.catalog.releaseNotes.toString().length > 0
                text: qsTr("Release Notes")
                onClicked: Qt.openUrlExternally(dialog.catalog.releaseNotes)
            }

            ProgressBar {
                Layout.fillWidth: true
                visible: dialog.phase !== "selection" || dialog.catalog.busy
                indeterminate: dialog.phase !== "download" || dialog.bytesTotal <= 0
                from: 0
                to: 1
                value: dialog.progress
            }

            Label {
                Layout.fillWidth: true
                visible: dialog.phase === "download"
                text: dialog.bytesTotal > 0 ? qsTr("%1 of %2 (%3%)").arg(dialog.formatBytes(dialog.bytesReceived)).arg(dialog.formatBytes(dialog.bytesTotal)).arg(Math.floor(dialog.progress * 100).toLocaleString(Qt.locale())) : dialog.formatBytes(dialog.bytesReceived)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: buttons.implicitHeight + 24
            color: Theme.backgroundSecondaryColor
            RowLayout {
                id: buttons
                anchors.fill: parent
                anchors.margins: 12
                Item { Layout.fillWidth: true }
                Button {
                    visible: dialog.phase === "selection"
                    enabled: dialog.catalog.available
                    highlighted: true
                    text: qsTr("Download")
                    onClicked: dialog.done("download")
                }
                Button {
                    text: qsTranslate("QPlatformTheme", "Cancel")
                    onClicked: dialog.finish("cancelled")
                }
            }
        }
    }
}
