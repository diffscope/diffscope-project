// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core
import DiffScope.Synth

WelcomeWizardPage {
    id: page
    title: qsTr("Synthesis Environment")
    description: qsTr("Configure a synthesis program or connect to a synthesis service")

    onAboutToEnter: helper.enter()
    onAboutToLeave: {
        page.forceActiveFocus()
        helper.leave()
    }

    SynthesisEnvironmentWelcomeWizardPageHelper {
        id: helper
    }

    ButtonGroup {
        id: environmentGroup
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RadioButton {
            text: qsTr("Use the local DiffSinger Synthesis Program")
            checked: helper.useLocal
            ButtonGroup.group: environmentGroup
            onClicked: helper.useLocal = true
        }
        RadioButton {
            text: qsTr("Connect to a synthesis service")
            checked: !helper.useLocal
            ButtonGroup.group: environmentGroup
            onClicked: helper.useLocal = false
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            DsspServerPage {
                visible: helper.useLocal
                pageHandle: helper.localSettings
                basicOptionsOnly: true
            }

            ServicesPage {
                id: servicesPage
                visible: !helper.useLocal
                pageHandle: helper
                configurationModel: helper.serviceModel
                onCurrentServiceChanged: helper.currentServiceId = servicesPage.currentService?.serviceId ?? ""
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Button {
                visible: helper.useLocal
                text: qsTr("More Options...")
                onClicked: Qt.callLater(() => {
                    page.forceActiveFocus()
                    CoreInterface.execSettingsDialog("org.diffscope.synth.DsspServer", page.Window.window)
                    helper.reloadLocalSettings()
                })
            }
            Button {
                visible: !helper.useLocal
                text: qsTr("Connect")
                onClicked: {
                    page.forceActiveFocus()
                    helper.connectServices()
                }
            }
            Label {
                Layout.fillWidth: true
                visible: !helper.useLocal && text.length > 0
                text: helper.connectionStatus
                textFormat: Text.PlainText
                color: helper.connectionFailed ? Theme.errorColor : Theme.foregroundSecondaryColor
                wrapMode: Text.Wrap
            }
            Item { Layout.fillWidth: true }
            Label {
                text: qsTr("Available singers: %1").arg(helper.singerCount.toLocaleString(Qt.locale(), 'f', 0))
            }
        }
    }
}
