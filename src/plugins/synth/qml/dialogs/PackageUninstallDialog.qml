// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

MessageBoxDialog {
    id: dialog

    required property string packageName
    required property var packages
    readonly property bool cascade: packages.length > 1

    width: 560
    title: cascade ? qsTr("Uninstall Packages") : qsTr("Uninstall Package")
    text: title
    informativeText: cascade
        ? qsTr("Other packages depend on %1. Uninstall this package and all packages listed below?").arg(packageName)
        : qsTr("Uninstall %1?").arg(packageName)
    textFormat: Text.PlainText
    icon: SVS.Question
    buttons: SVS.Yes | SVS.No
    primaryButton: SVS.No
    escapeButton: SVS.No

    ScrollView {
        id: packageScroll
        visible: dialog.cascade
        implicitHeight: visible ? Math.min(240, packageList.implicitHeight) : 0
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
