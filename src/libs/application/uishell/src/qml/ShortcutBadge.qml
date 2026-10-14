// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQuick

import SVSCraft
import SVSCraft.UIComponents

Rectangle {
    id: control

    property string text: ""
    property font font: Theme.font
    property color textColor: Theme.foregroundPrimaryColor
    property real horizontalPadding: 4
    property real verticalPadding: 2

    implicitWidth: label.implicitWidth + horizontalPadding * 2
    implicitHeight: label.implicitHeight + verticalPadding * 2
    color: Theme.buttonColor
    border.color: Theme.borderColor
    radius: 2
    Accessible.role: Accessible.StaticText
    Accessible.name: text

    Text {
        id: label
        anchors.centerIn: parent
        width: Math.max(0, control.width - control.horizontalPadding * 2)
        height: Math.max(0, control.height - control.verticalPadding * 2)
        text: control.text
        font: control.font
        color: control.textColor
        textFormat: Text.PlainText
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
