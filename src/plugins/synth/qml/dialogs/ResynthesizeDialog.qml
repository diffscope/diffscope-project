// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Dialog {
    id: dialog

    property int resynthesizeFrom
    property bool disableCache

    width: 480
    title: qsTr("Resynthesize")
    standardButtons: DialogButtonBox.Ok

    onAboutToShow: resynthesizeFromComboBox.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        GridLayout {
            Layout.fillWidth: true
            columns: 2

            Label {
                id: resynthesizeFromLabel
                text: qsTr("Resynthesize from")
            }
            ComboBox {
                id: resynthesizeFromComboBox
                Accessible.labelledBy: resynthesizeFromLabel
                Accessible.name: resynthesizeFromLabel.text
                Layout.fillWidth: true
                model: [qsTr("Pronunciation"), qsTr("Phoneme"), qsTr("Duration"), qsTr("Parameter"), qsTr("Audio")]
                currentIndex: dialog.resynthesizeFrom
                onActivated: (index) => dialog.resynthesizeFrom = index
            }

            Label {
                Layout.columnSpan: 2
                Layout.fillWidth: true
                ThemedItem.foregroundLevel: SVS.FL_Secondary
                wrapMode: Text.Wrap
                text: qsTr("Cascading synthesis tasks will also be resynthesized.")
            }

            CheckBox {
                Layout.columnSpan: 2
                text: qsTr("Disable cache")
                checked: dialog.disableCache
                onClicked: dialog.disableCache = checked
            }
        }
    }
}
