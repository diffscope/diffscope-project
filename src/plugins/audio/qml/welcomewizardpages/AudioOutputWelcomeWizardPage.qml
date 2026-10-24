// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

WelcomeWizardPage {
    id: page
    title: qsTr("Audio Output")
    description: qsTr("Configure audio output device parameters")
    iconSource: "image://fluent-system-icons/speaker_2?style=regular"

    onAboutToEnter: helper.reload()

    AudioOutputSettingsHelper {
        id: helper
    }

    ColumnLayout {
        spacing: 16
        anchors.centerIn: parent
        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 600
            columns: 2
            Label {
                text: qsTr("Audio driver")
            }
            ComboBox {
                Layout.fillWidth: true
                model: helper.driverList
                currentIndex: helper.driverCurrentIndex
                onActivated: (index) => helper.driverCurrentIndex = index
            }
            Label {
                text: qsTr("Audio device")
            }
            ComboBox {
                Layout.fillWidth: true
                model: helper.deviceList
                currentIndex: helper.deviceCurrentIndex
                onActivated: (index) => helper.deviceCurrentIndex = index
            }
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            Button {
                text: qsTr("Test")
                onClicked: helper.testDevice()
            }
            Button {
                text: qsTr("More Options...")
                onClicked: Qt.callLater(() => {
                    page.forceActiveFocus()
                    CoreInterface.execSettingsDialog("audio.AudioOutput", page.Window.window)
                    helper.reload()
                })
            }
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            ThemedItem.foregroundLevel: SVS.FL_Secondary
            text: qsTr('Click "Test" button to check whether audio output is configured correctly')
        }
    }
}
