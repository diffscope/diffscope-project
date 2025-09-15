import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

WelcomeWizardPage {
    id: page
    title: qsTr("Color Scheme")
    description: qsTr("Choose a color scheme for %1").arg(Application.name)
    Rectangle {
        anchors.fill: parent
        color: "red"
    }
}