// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

MessageBoxDialog {
    id: dialog

    required property int newWindowButton
    required property int currentWindowButton
    readonly property alias rememberChoice: rememberCheckBox.checked

    text: qsTr("Open Project")
    informativeText: qsTr("Where do you want to open the project?")
    icon: SVS.Question
    buttons: [
        { id: dialog.newWindowButton, text: qsTr("Open in New Window"), role: DialogButtonBox.AcceptRole },
        { id: dialog.currentWindowButton, text: qsTr("Open in Current Window"), role: DialogButtonBox.AcceptRole },
        SVS.Cancel
    ]
    primaryButton: newWindowButton

    content: CheckBox {
        id: rememberCheckBox
        text: qsTr("Do not ask again")
    }
}
