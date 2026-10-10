// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Synth

Dialog {
    id: dialog

    required property AudioExtractionAddOn addOn
    required property AudioExtractionTask task
    required property bool separation
    property string directory
    property string scenarioError
    readonly property bool busy: task?.busy ?? false
    readonly property bool refreshing: task?.refreshing ?? false
    readonly property var extractors: task?.extractors ?? []
    readonly property var extractor: extractors[extractorBox.currentIndex] ?? null

    width: 520
    modal: true
    closePolicy: Popup.NoAutoClose
    title: separation ? qsTr("Separate Audio") : qsTr("Extract Notes")
    onAboutToShow: extractorBox.forceActiveFocus()
    onRejected: task?.cancel()
    onClosed: addOn?.closeDialog()

    footer: DialogButtonBox {
        Button {
            text: qsTr("Start")
            enabled: dialog.task !== null && !dialog.busy && !dialog.refreshing && dialog.extractor !== null
                     && (!dialog.separation || dialog.directory.length > 0)
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: dialog.addOn.start(extractorBox.currentIndex, dialog.directory)
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onRejected: dialog.reject()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        GridLayout {
            Layout.fillWidth: true
            enabled: dialog.task !== null && !dialog.busy
            columns: 2
            columnSpacing: 12
            rowSpacing: 8

            Label {
                id: extractorLabel
                text: qsTr("Extractor")
            }
            RowLayout {
                Layout.fillWidth: true
                ComboBox {
                    id: extractorBox
                    Layout.fillWidth: true
                    enabled: !dialog.refreshing
                    model: dialog.extractors
                    textRole: "name"
                    Accessible.name: extractorLabel.text
                    Accessible.labelledBy: extractorLabel
                }
                ToolButton {
                    text: qsTr("Refresh")
                    display: AbstractButton.IconOnly
                    icon.source: "image://fluent-system-icons/arrow_sync"
                    enabled: !dialog.refreshing
                    onClicked: dialog.task?.refresh()
                }
            }

            Item {
                visible: trackLayoutLabel.visible
            }
            Label {
                id: trackLayoutLabel
                Layout.fillWidth: true
                visible: dialog.separation && text.length > 0
                text: qsTr("Track layout: ") + (dialog.extractor?.trackLayout ?? "")
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }

            Label {
                id: directoryLabel
                visible: dialog.separation
                text: qsTr("Output directory")
            }
            RowLayout {
                Layout.fillWidth: true
                visible: dialog.separation
                TextField {
                    Layout.fillWidth: true
                    text: dialog.directory
                    selectByMouse: true
                    Accessible.name: directoryLabel.text
                    Accessible.labelledBy: directoryLabel
                    onTextEdited: dialog.directory = text
                    onEditingFinished: dialog.directory = dialog.addOn.normalizedPath(text)
                }
                ToolButton {
                    text: qsTr("Browse...")
                    display: AbstractButton.IconOnly
                    icon.source: "image://fluent-system-icons/folder_open"
                    onClicked: {
                        const directory = dialog.addOn.chooseDirectory(dialog.directory)
                        if (directory.length > 0)
                            dialog.directory = directory
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: dialog.busy || dialog.refreshing
            text: dialog.busy ? qsTr("Extracting audio…") : qsTr("Updating extractors…")
            wrapMode: Text.Wrap
        }
        ProgressBar {
            Layout.fillWidth: true
            visible: dialog.busy || dialog.refreshing
            indeterminate: true
            Accessible.name: qsTr("Extraction status")
        }
        Label {
            Layout.fillWidth: true
            visible: text.length > 0
            text: dialog.scenarioError.length > 0 ? dialog.scenarioError : (dialog.task?.error ?? "")
            wrapMode: Text.Wrap
        }
    }
}
