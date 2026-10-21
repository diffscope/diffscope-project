// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Window {
    id: dialog

    property string text: ""
    property bool cancellable: true
    property bool completed: false
    property var finalResult

    signal done(var result)

    function finish(result) {
        if (completed)
            return
        completed = true
        finalResult = result
        done(result)
        close()
    }

    title: qsTr("LibreSVIP")
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
    modality: Qt.ApplicationModal
    width: 480
    minimumHeight: layout.implicitHeight
    maximumHeight: layout.implicitHeight
    height: layout.implicitHeight

    onClosing: (event) => {
        if (!completed) {
            if (cancellable) {
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
                    text: dialog.text
                    wrapMode: Text.Wrap
                    font.pixelSize: 16
                }

                ProgressBar {
                    Layout.fillWidth: true
                    indeterminate: true
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
                    visible: dialog.cancellable
                    text: qsTranslate("QPlatformTheme", "Cancel")
                    onClicked: dialog.finish("cancelled")
                }
            }
        }
    }
}
