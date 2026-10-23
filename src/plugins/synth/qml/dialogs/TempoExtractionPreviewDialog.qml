// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Synth

Dialog {
    id: dialog

    required property AudioExtractionAddOn addOn
    required property TempoExtractionPreview preview
    property string scenarioError

    width: Math.min(960, (parent?.width ?? 992) - 32)
    modal: true
    closePolicy: Popup.NoAutoClose
    title: qsTr("Extract Tempo")
    onOpened: previewView.focusFirstControl()
    onRejected: preview.pause()
    onClosed: addOn.closeDialog()
    onWidthChanged: if (parent) x = (parent.width - width) / 2

    footer: DialogButtonBox {
        Button {
            text: qsTr("Apply")
            enabled: dialog.preview.valid
            ThemedItem.controlType: SVS.CT_Accent
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: dialog.addOn.applyTempo()
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onRejected: dialog.reject()
    }

    contentItem: ColumnLayout {
        id: pages
        spacing: 12

        ScrollView {
            id: previewScroll
            Layout.fillWidth: true
            implicitHeight: Math.max(0, Math.min(previewView.implicitHeight,
                                                 (dialog.parent?.height ?? 900) - 32 - dialog.topPadding - dialog.bottomPadding
                                                 - (dialog.header?.height ?? 0) - (dialog.footer?.height ?? 0) - 2 * dialog.spacing
                                                 - (scenarioErrorLabel.visible ? scenarioErrorLabel.implicitHeight + pages.spacing : 0)))
            leftPadding: mirrored ? effectiveScrollBarWidth : 0
            rightPadding: mirrored ? 0 : effectiveScrollBarWidth
            contentWidth: availableWidth
            contentHeight: previewView.implicitHeight
            clip: true

            TempoExtractionPreviewView {
                id: previewView
                width: previewScroll.availableWidth
                height: implicitHeight
                preview: dialog.preview
            }
        }

        Label {
            id: scenarioErrorLabel
            Layout.fillWidth: true
            visible: text.length > 0
            text: dialog.scenarioError
            color: Theme.errorColor
            wrapMode: Text.Wrap
        }
    }
}
