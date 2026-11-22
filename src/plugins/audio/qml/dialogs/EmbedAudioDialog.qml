// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Dialog {
    id: dialog
    property bool preparing: false
    readonly property bool lossy: lossyOption.checked
    title: preparing ? qsTr("Embedding Audio") : qsTr("Embed Audio")
    width: preparing ? implicitWidth : Math.min(440, parent?.width ?? 440)
    modal: true
    closePolicy: Popup.CloseOnEscape
    standardButtons: preparing ? DialogButtonBox.Cancel : DialogButtonBox.Ok | DialogButtonBox.Cancel
    onAboutToShow: {
        if (!preparing)
            losslessOption.forceActiveFocus()
    }

    component OptionDescription: Label {
        required property RadioButton option
        readonly property real indent: (option.mirrored ? option.rightPadding : option.leftPadding) + option.implicitIndicatorWidth + option.spacing
        Layout.fillWidth: true
        leftPadding: option.mirrored ? 0 : indent
        rightPadding: option.mirrored ? indent : 0
        wrapMode: Text.Wrap
        ThemedItem.foregroundLevel: SVS.FL_Secondary
    }

    contentItem: ColumnLayout {
        spacing: 0
        ButtonGroup { id: encoding }

        RadioButton {
            id: losslessOption
            text: qsTr("Lossless")
            checked: true
            visible: !dialog.preparing
            ButtonGroup.group: encoding
        }

        OptionDescription {
            option: losslessOption
            visible: !dialog.preparing
            text: qsTr("Lossless embedding preserves the source audio quality but usually requires more storage space. Choose lossless embedding for audio included in the final export.")
        }

        RadioButton {
            id: lossyOption
            Layout.topMargin: 8
            text: qsTr("Lossy")
            visible: !dialog.preparing
            ButtonGroup.group: encoding
        }

        OptionDescription {
            option: lossyOption
            visible: !dialog.preparing
            text: qsTr("Lossy embedding usually requires less storage space but discards some audio detail. Choose lossy embedding for audio used only as a reference.")
        }

        ProgressBar {
            Layout.fillWidth: true
            visible: dialog.preparing
            indeterminate: true
            Accessible.name: qsTr("Encoding audio")
        }
    }
}
