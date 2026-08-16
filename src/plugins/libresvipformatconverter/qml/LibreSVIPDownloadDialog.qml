// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Window {
    id: dialog

    property string phase: "catalog"
    property string statusText: ""
    property var versionItems: []
    property int selectedIndex: 0
    property bool hasDownloadedInstallation: false
    property bool indeterminate: true
    property real progress: 0
    property double bytesReceived: 0
    property double bytesTotal: -1
    property bool completed: false
    property var finalResult
    property var escapeButton: SVS.Cancel

    signal done(var result)

    function formatBytes(value) {
        const units = [qsTr("B"), qsTr("KiB"), qsTr("MiB"), qsTr("GiB"), qsTr("TiB")]
        let size = Math.max(0, value)
        let unit = 0
        while (size >= 1024 && unit < units.length - 1) {
            size /= 1024
            ++unit
        }
        return Number(size.toFixed(unit === 0 ? 0 : 1)).toLocaleString(Qt.locale()) + " " + units[unit]
    }

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

    title: qsTr("LibreSVIP")
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
           | (escapeButton === SVS.Cancel ? Qt.WindowCloseButtonHint : 0)
    modality: Qt.ApplicationModal
    width: 480
    minimumHeight: layout.implicitHeight
    maximumHeight: layout.implicitHeight
    height: layout.implicitHeight

    onClosing: (event) => {
        if (!completed) {
            if (escapeButton === SVS.Cancel) {
                completed = true
                finalResult = "cancelled"
                done("cancelled")
                event.accepted = true
            } else {
                event.accepted = false
            }
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

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 16

            Image {
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: 64
                Layout.preferredHeight: 64
                sourceSize.width: 64
                sourceSize.height: 64
                source: "qrc:/libresvipformatconverter/res/libresvip/libresvip.ico"
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    text: {
                        switch (dialog.phase) {
                        case "catalog":
                            return qsTr("Retrieving available LibreSVIP versions...")
                        case "selection":
                            return qsTr("Select the LibreSVIP version to download:")
                        case "download":
                            return qsTr("Downloading LibreSVIP...")
                        case "install":
                            return dialog.statusText
                        default:
                            return ""
                        }
                    }
                    wrapMode: Text.Wrap
                    font.pixelSize: 16
                }

                RowLayout {
                    visible: dialog.phase === "selection"
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("Version")
                    }

                    ComboBox {
                        id: versionComboBox
                        Layout.fillWidth: true
                        model: dialog.versionItems
                        currentIndex: dialog.selectedIndex
                        onActivated: dialog.selectedIndex = currentIndex
                    }
                }

                ProgressBar {
                    visible: dialog.phase === "catalog" || dialog.phase === "download" || dialog.phase === "install"
                    Layout.fillWidth: true
                    indeterminate: dialog.indeterminate
                    from: 0
                    to: 1
                    value: dialog.progress
                }

                RowLayout {
                    visible: dialog.phase === "download"
                    Layout.fillWidth: true

                    Label {
                        text: dialog.formatBytes(dialog.bytesReceived)
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        visible: dialog.bytesTotal > 0
                        text: qsTr("%1% — %2").arg(Math.floor(dialog.progress * 100)).arg(dialog.formatBytes(dialog.bytesTotal))
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 52
            color: Theme.backgroundSecondaryColor

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12

                Item { Layout.fillWidth: true }

                Button {
                    visible: dialog.phase === "selection"
                    enabled: versionComboBox.currentIndex >= 0
                    highlighted: true
                    text: dialog.hasDownloadedInstallation ? qsTr("Update") : qsTr("Download")
                    onClicked: {
                        dialog.selectedIndex = versionComboBox.currentIndex
                        dialog.done("download")
                    }
                }

                Button {
                    visible: dialog.escapeButton === SVS.Cancel
                    text: qsTranslate("QPlatformTheme", "Cancel")
                    onClicked: dialog.finish("cancelled")
                }

            }
        }
    }
}
