// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Synth

WelcomeWizardPage {
    id: page
    title: qsTr("Package Manager")
    description: qsTr("Install sound libraries and other resources")
    iconSource: "image://fluent-system-icons/box?style=regular"
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    PackageManagerWelcomeWizardPageHelper {
        id: helper
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 32
        contentWidth: availableWidth

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 16

            Label {
                Layout.fillWidth: true
                text: qsTr("DiffScope Package (.dspk) is a format for distributing sound libraries and other resources for DiffScope, including model weights and shared resources required for synthesis.")
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Synthesis with the local DiffSinger Synthesis Program requires packages containing sound libraries and their dependencies. Install these packages with the Package Manager.")
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }

            Button {
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Open Package Manager")
                onClicked: Qt.callLater(() => helper.openPackageManager(page.Window.window))
            }
        }
    }
}
