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
    property alias directory: separationOptions.directory
    property string scenarioError
    readonly property bool busy: task?.busy ?? false
    readonly property bool refreshing: task?.refreshing ?? false
    readonly property var extractors: task?.extractors ?? []
    readonly property var extractor: extractors[extractorBox.currentIndex] ?? null

    width: Math.min(520, (parent?.width ?? 552) - 32)
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
                     && (!dialog.separation || (dialog.directory.length > 0 && separationOptions.routingValid))
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: dialog.addOn.start(extractorBox.currentIndex, dialog.directory,
                                         separationOptions.outputTrackCount, separationOptions.outputRoutes)
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onRejected: dialog.reject()
    }

    contentItem: ScrollView {
        id: scrollView
        implicitHeight: Math.max(0, Math.min(content.implicitHeight,
                                             (dialog.parent?.height ?? 900) - 32 - dialog.topPadding - dialog.bottomPadding
                                             - (dialog.header?.height ?? 0) - (dialog.footer?.height ?? 0) - 2 * dialog.spacing))
        leftPadding: mirrored ? effectiveScrollBarWidth : 0
        rightPadding: mirrored ? 0 : effectiveScrollBarWidth
        contentWidth: availableWidth
        contentHeight: content.implicitHeight
        clip: true

        ColumnLayout {
            id: content
            width: scrollView.availableWidth
            height: implicitHeight
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
            }

            AudioSeparationOptions {
                id: separationOptions
                Layout.fillWidth: true
                visible: dialog.separation
                enabled: dialog.task !== null && !dialog.busy && !dialog.refreshing
                addOn: dialog.addOn
                trackNames: dialog.extractor?.trackNames ?? []
                onControlFocused: control => {
                    const top = control.mapToItem(content, 0, 0).y
                    if (top < scrollView.contentItem.contentY)
                        scrollView.contentItem.contentY = top
                    else if (top + control.height > scrollView.contentItem.contentY + scrollView.availableHeight)
                        scrollView.contentItem.contentY = top + control.height - scrollView.availableHeight
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
                value: dialog.task?.progress ?? 0
                indeterminate: dialog.refreshing || value === 0
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
}
