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
    title: qsTr("Achievements (Experimental Feature)")
    description: qsTr("Achievements provide tips about %1 in an interactive way").arg(Application.name)

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 32
        Card {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 400
            implicitHeight: 60
            Component.onCompleted: () => {
                background.color = Qt.binding(() => Theme.backgroundTertiaryColor)
            }
            title: Label {
                text: qsTr("Achievement Completed")
                font.weight: Font.DemiBold
                color: Theme.accentColor
            }
            subtitle: Label {
                text: qsTr("Welcome to DiffScope")
                color: Theme.foregroundPrimaryColor
            }
            image: Rectangle {
                color: Theme.backgroundQuaternaryColor
                IconLabel {
                    anchors.centerIn: parent
                    icon.source: "image://appicon/app"
                    icon.color: "transparent"
                    icon.width: 32
                    icon.height: 32
                }
            }
            toolBar: RowLayout {
                IconLabel {
                    icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/CheckmarkCircle16Filled.svg"
                    icon.color: Theme.accentColor
                }
            }
        }
        CheckBox {
            id: checkBox
            Layout.alignment: Qt.AlignHCenter
            text: qsTr('Show "Achievement Completed" notification')
            checked: true
            Settings {
                id: s
                settings: RuntimeInterface.settings
                category: "org.diffscope.achievements"
                property alias _showNotification: checkBox.checked
            }
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            ThemedItem.foregroundLevel: SVS.FL_Secondary
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Even if notification is disabled, completed achievements will still be recorded\n\nYou can view all achievements in Help > Achievements")
        }
    }
}