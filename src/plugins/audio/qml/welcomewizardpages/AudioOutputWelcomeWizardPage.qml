import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.Extras

import ChorusKit.AppCore

import DiffScope.UIShell

WelcomeWizardPage {
    id: page
    title: qsTr("Audio Output")
    description: qsTr("Configure audio output device parameters")

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
            }
            Label {
                text: qsTr("Audio device")
            }
            ComboBox {
                Layout.fillWidth: true
                model: helper.deviceList
            }
            Label {
                text: qsTr("Sample rate")
            }
            ComboBox {
                Layout.fillWidth: true
                model: helper.sampleRateList
            }
            Label {
                text: qsTr("Buffer size")
            }
            ComboBox {
                Layout.fillWidth: true
                model: helper.bufferSizeList
            }
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            Button {
                text: qsTr("Test")
            }
            Button {
                text: qsTr("Open Control Panel")
            }
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            ThemedItem.foregroundLevel: SVS.FL_Secondary
            text: qsTr('Click "Test" button to check whether audio output is configured correctly')
        }
    }
}