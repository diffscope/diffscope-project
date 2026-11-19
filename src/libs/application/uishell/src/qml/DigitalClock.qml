// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents

T.Button {
    id: control
    property bool backgroundVisible: true
    property int fineTuneMode: DigitalClockLabel.TabularFiguresFontFeature
    property string secondaryText

    implicitWidth: Math.max(backgroundVisible ? 108 : 0, timeRow.implicitWidth + 16)
    implicitHeight: 24
    Accessible.name: qsTr("Digital clock") + "\n" + text + (secondaryText ? "\n" + secondaryText : "")
    hoverEnabled: true

    background: Rectangle {
        visible: control.backgroundVisible
        color: Theme.backgroundTertiaryColor
        border.color: Theme.borderColor
        radius: 4
    }

    component TimeLabel: DigitalClockLabel {
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.foregroundPrimaryColor
        font.family: Theme.font.family
        font.pixelSize: 16
        fineTuneMode: control.fineTuneMode
    }

    Row {
        id: timeRow
        anchors.centerIn: parent
        height: control.height
        spacing: 8

        TimeLabel {
            text: control.text
        }

        Rectangle {
            visible: secondaryTimeLabel.visible
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 4
            anchors.bottomMargin: 4
            width: 1
            color: Theme.borderColor
        }

        TimeLabel {
            id: secondaryTimeLabel
            visible: control.secondaryText.length > 0
            text: control.secondaryText
        }
    }
}
