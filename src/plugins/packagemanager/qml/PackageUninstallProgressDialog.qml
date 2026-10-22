// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Window {
    id: dialog

    required property var packages
    property string statusText: qsTr("Preparing package uninstallation...")

    title: qsTr("Uninstall Packages")
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
    modality: Qt.ApplicationModal
    width: 560
    minimumWidth: 480
    height: layout.implicitHeight + 32
    minimumHeight: layout.implicitHeight + 32
    maximumHeight: layout.implicitHeight + 32

    onClosing: (event) => event.accepted = false

    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundPrimaryColor
    }

    ColumnLayout {
        id: layout
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: dialog.statusText
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
        }

        ProgressBar {
            Layout.fillWidth: true
            indeterminate: true
            Accessible.name: dialog.statusText
        }

        ScrollView {
            id: packageScroll
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(200, packageList.implicitHeight)
            contentWidth: availableWidth
            clip: true

            TextArea {
                id: packageList
                width: packageScroll.availableWidth
                text: dialog.packages.join("\n")
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                readOnly: true
                selectByMouse: true
                activeFocusOnTab: true
                Accessible.name: qsTr("Packages to uninstall")
            }
        }
    }
}
