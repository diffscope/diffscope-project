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
    required property string installationName
    required property string directory
    property string preparationText: ""
    property bool completed: false
    property var finalResult
    property var escapeButton: SVS.Cancel

    signal done(var result)

    function finish(result) {
        if (completed)
            return
        completed = true
        finalResult = result
        done(result)
        close()
    }

    title: qsTr("Uninstall %1").arg(dialog.productName)
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    width: 560
    minimumWidth: 480
    minimumHeight: layout.implicitHeight
    maximumHeight: layout.implicitHeight
    height: layout.implicitHeight

    onClosing: {
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
            spacing: 12

            Label {
                Layout.fillWidth: true
                text: qsTr("Uninstall %1?").arg(dialog.installationName)
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("All files in this installation directory will be deleted:")
                wrapMode: Text.Wrap
            }
            TextField {
                Layout.fillWidth: true
                text: dialog.directory
                readOnly: true
                selectByMouse: true
                Accessible.name: qsTr("Installation directory")
            }
            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: dialog.preparationText
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
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
                    text: qsTr("Uninstall")
                    onClicked: dialog.finish("uninstall")
                }
                Button {
                    text: qsTranslate("QPlatformTheme", "Cancel")
                    focus: true
                    onClicked: dialog.finish("cancelled")
                }
            }
        }
    }
}
