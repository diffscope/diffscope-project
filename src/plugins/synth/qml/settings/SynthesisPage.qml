// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

ScrollView {
    id: page

    required property QtObject pageHandle
    readonly property TextMatcher matcher: TextMatcher {}
    anchors.fill: parent
    contentWidth: availableWidth

    function formatCacheSize(bytes) {
        const value = Number(bytes)
        if (value < 1024)
            return qsTr("%1 B").arg(Math.round(value).toLocaleString(Qt.locale()))
        if (value < 1024 * 1024)
            return qsTr("%1 KiB").arg((value / 1024).toLocaleString(Qt.locale(), "f", 1))
        if (value < 1024 * 1024 * 1024)
            return qsTr("%1 MiB").arg((value / (1024 * 1024)).toLocaleString(Qt.locale(), "f", 1))
        return qsTr("%1 GiB").arg((value / (1024 * 1024 * 1024)).toLocaleString(Qt.locale(), "f", 2))
    }

    Dialog {
        id: clearCacheDialog

        property var sizes: ({})

        function updateClearButton() {
            const button = standardButton(DialogButtonBox.Ok)
            if (button)
                button.enabled = pronunciationCheckBox.checked || phonemeCheckBox.checked || durationCheckBox.checked || parameterCheckBox.checked || audioCheckBox.checked
        }

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(480, parent ? parent.width - 48 : 0)
        modal: true
        title: qsTr("Clear Synthesis Cache")
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onAboutToShow: {
            sizes = page.pageHandle.cacheSizes()
            pronunciationCheckBox.checked = true
            phonemeCheckBox.checked = true
            durationCheckBox.checked = true
            parameterCheckBox.checked = true
            audioCheckBox.checked = true
        }
        onOpened: {
            const button = standardButton(DialogButtonBox.Ok)
            if (button)
                button.text = qsTr("Clear Selected")
            updateClearButton()
            pronunciationCheckBox.forceActiveFocus()
        }
        onAccepted: {
            const taskTypes = []
            if (pronunciationCheckBox.checked)
                taskTypes.push("pronunciation")
            if (phonemeCheckBox.checked)
                taskTypes.push("phoneme")
            if (durationCheckBox.checked)
                taskTypes.push("duration")
            if (parameterCheckBox.checked)
                taskTypes.push("parameter")
            if (audioCheckBox.checked)
                taskTypes.push("audio")
            page.pageHandle.clearCache(taskTypes)
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: qsTr("Select the synthesis task caches to delete.")
                wrapMode: Text.Wrap
            }
            CheckBox {
                id: pronunciationCheckBox
                Layout.fillWidth: true
                text: qsTr("Pronunciation (%1)").arg(page.formatCacheSize(clearCacheDialog.sizes["pronunciation"] || 0))
                onToggled: clearCacheDialog.updateClearButton()
            }
            CheckBox {
                id: phonemeCheckBox
                Layout.fillWidth: true
                text: qsTr("Phoneme (%1)").arg(page.formatCacheSize(clearCacheDialog.sizes["phoneme"] || 0))
                onToggled: clearCacheDialog.updateClearButton()
            }
            CheckBox {
                id: durationCheckBox
                Layout.fillWidth: true
                text: qsTr("Duration (%1)").arg(page.formatCacheSize(clearCacheDialog.sizes["duration"] || 0))
                onToggled: clearCacheDialog.updateClearButton()
            }
            CheckBox {
                id: parameterCheckBox
                Layout.fillWidth: true
                text: qsTr("Parameter (%1)").arg(page.formatCacheSize(clearCacheDialog.sizes["parameter"] || 0))
                onToggled: clearCacheDialog.updateClearButton()
            }
            CheckBox {
                id: audioCheckBox
                Layout.fillWidth: true
                text: qsTr("Audio (%1)").arg(page.formatCacheSize(clearCacheDialog.sizes["audio"] || 0))
                onToggled: clearCacheDialog.updateClearButton()
            }
        }
    }

    Dialog {
        id: clearDiagnosticsDialog

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(480, parent ? parent.width - 48 : 0)
        modal: true
        title: qsTr("Clear Synthesis Diagnostics")
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onOpened: {
            const button = standardButton(DialogButtonBox.Ok)
            if (button) {
                button.text = qsTr("Clear")
                button.forceActiveFocus()
            }
        }
        onAccepted: page.pageHandle.clearDiagnostics()

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: qsTr("Delete all saved synthesis diagnostics from the disk?")
                wrapMode: Text.Wrap
            }
        }
    }

    ColumnLayout {
        width: page.width

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32

            GroupBox {
                Layout.fillWidth: true
                title: qsTr("Synthesis")
                TextMatcherItem on title {
                    matcher: page.matcher
                }

                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 8

                    Label {
                        id: basePiecePaddingLabel
                        readonly property string description: qsTr("Padding added before and after each synthesis piece.")
                        text: qsTr("Base piece padding (ms)")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: basePiecePaddingHoverHandler.hovered
                        HoverHandler { id: basePiecePaddingHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    SpinBox {
                        from: 0
                        to: 60000
                        editable: true
                        value: page.pageHandle.paddingBase
                        onValueModified: page.pageHandle.paddingBase = value
                        Accessible.description: basePiecePaddingLabel.description
                    }
                    Label {
                        id: additionalOnsetPaddingLabel
                        readonly property string description: qsTr("Additional leading padding added for each phoneme before a note onset.")
                        text: qsTr("Additional onset padding (ms)")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: additionalOnsetPaddingHoverHandler.hovered
                        HoverHandler { id: additionalOnsetPaddingHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    SpinBox {
                        from: 0
                        to: 60000
                        editable: true
                        value: page.pageHandle.paddingAdditional
                        onValueModified: page.pageHandle.paddingAdditional = value
                        Accessible.description: additionalOnsetPaddingLabel.description
                    }
                    Label {
                        id: pieceGapThresholdLabel
                        readonly property string description: qsTr("Minimum gap used to separate note groups into synthesis pieces and limit leading padding.")
                        text: qsTr("Piece gap threshold (ms)")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: pieceGapThresholdHoverHandler.hovered
                        HoverHandler { id: pieceGapThresholdHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    SpinBox {
                        from: 0
                        to: 60000
                        editable: true
                        value: page.pageHandle.paddingGap
                        onValueModified: page.pageHandle.paddingGap = value
                        Accessible.description: pieceGapThresholdLabel.description
                    }
                    Label {
                        id: restLyricsLabel
                        readonly property string description: qsTr("Comma-separated lyrics treated as rests when dividing a clip into synthesis pieces.")
                        text: qsTr("Rest lyrics")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: restLyricsHoverHandler.hovered
                        HoverHandler { id: restLyricsHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: page.pageHandle.restLyrics
                        placeholderText: qsTr("Comma-separated, for example AP, SP")
                        onTextEdited: page.pageHandle.restLyrics = text
                        Accessible.description: restLyricsLabel.description
                    }
                    Label {
                        id: parameterSampleRateLabel
                        readonly property string description: qsTr("Number of parameter samples generated per second.")
                        text: qsTr("Parameter sample rate (Hz)")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: parameterSampleRateHoverHandler.hovered
                        HoverHandler { id: parameterSampleRateHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    SpinBox {
                        from: 1
                        to: 1000
                        editable: true
                        value: page.pageHandle.parameterSampleRate
                        onValueModified: page.pageHandle.parameterSampleRate = value
                        Accessible.description: parameterSampleRateLabel.description
                    }
                    Label {
                        id: singerMixSampleRateLabel
                        readonly property string description: qsTr("Number of singer-mix samples generated per second.")
                        text: qsTr("Singer mix sample rate (Hz)")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: singerMixSampleRateHoverHandler.hovered
                        HoverHandler { id: singerMixSampleRateHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    SpinBox {
                        from: 1
                        to: 1000
                        editable: true
                        value: page.pageHandle.mixSampleRate
                        onValueModified: page.pageHandle.mixSampleRate = value
                        Accessible.description: singerMixSampleRateLabel.description
                    }
                }
            }

            GroupBox {
                Layout.fillWidth: true
                title: qsTr("Cache")
                TextMatcherItem on title {
                    matcher: page.matcher
                }

                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 8

                    Label {
                        id: maximumCacheSizeLabel
                        readonly property string description: qsTr("Maximum disk space used by synthesis task caches.")
                        text: qsTr("Maximum cache size (GiB)")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: maximumCacheSizeHoverHandler.hovered
                        HoverHandler { id: maximumCacheSizeHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    SpinBox {
                        from: 1
                        to: 1024
                        editable: true
                        value: page.pageHandle.cacheMaximumGiB
                        onValueModified: page.pageHandle.cacheMaximumGiB = value
                        Accessible.description: maximumCacheSizeLabel.description
                    }
                    Label {
                        id: cacheExpiryLabel
                        readonly property string description: qsTr("Number of days before an unused cache entry expires. Use 0 to disable expiry.")
                        text: qsTr("Cache expiry (days)")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: cacheExpiryHoverHandler.hovered
                        HoverHandler { id: cacheExpiryHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    SpinBox {
                        from: 0
                        to: 3650
                        editable: true
                        value: page.pageHandle.cacheExpiryDays
                        onValueModified: page.pageHandle.cacheExpiryDays = value
                        Accessible.description: cacheExpiryLabel.description
                    }
                    Label {
                        id: maximumAudioDownloadSizeLabel
                        readonly property string description: qsTr("Maximum size of a synthesized audio file downloaded from a service.")
                        text: qsTr("Maximum audio download size (MiB)")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: maximumAudioDownloadSizeHoverHandler.hovered
                        HoverHandler { id: maximumAudioDownloadSizeHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    SpinBox {
                        from: 1
                        to: 16384
                        editable: true
                        value: page.pageHandle.audioDownloadMaximumMiB
                        onValueModified: page.pageHandle.audioDownloadMaximumMiB = value
                        Accessible.description: maximumAudioDownloadSizeLabel.description
                    }
                    Label {
                        id: environmentTagLifetimeLabel
                        readonly property string description: qsTr("How long to reuse a service-provided synthesis environment tag before requesting a new one.")
                        text: qsTr("Environment tag lifetime (seconds)")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: environmentTagLifetimeHoverHandler.hovered
                        HoverHandler { id: environmentTagLifetimeHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    SpinBox {
                        from: 1
                        to: 86400
                        editable: true
                        value: page.pageHandle.environmentTagTtlSeconds
                        onValueModified: page.pageHandle.environmentTagTtlSeconds = value
                        Accessible.description: environmentTagLifetimeLabel.description
                    }
                    Button {
                        Layout.columnSpan: 2
                        text: qsTr("Clear Synthesis Cache")
                        icon.source: "image://fluent-system-icons/delete"
                        onClicked: clearCacheDialog.open()
                    }
                }
            }

            GroupBox {
                Layout.fillWidth: true
                title: qsTr("Diagnostics")
                TextMatcherItem on title {
                    matcher: page.matcher
                }

                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 8

                    Label {
                        id: maximumDiagnosticsSizeLabel
                        readonly property string description: qsTr("Maximum disk space used by synthesis diagnostics.")
                        text: qsTr("Maximum diagnostics size (MiB)")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: maximumDiagnosticsSizeHoverHandler.hovered
                        HoverHandler { id: maximumDiagnosticsSizeHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    SpinBox {
                        from: 1
                        to: 1048576
                        editable: true
                        value: page.pageHandle.diagnosticsMaximumMiB
                        onValueModified: page.pageHandle.diagnosticsMaximumMiB = value
                        Accessible.description: maximumDiagnosticsSizeLabel.description
                    }
                    Label {
                        id: diagnosticsExpiryLabel
                        readonly property string description: qsTr("Number of days before a diagnostics file expires. Use 0 to disable expiry.")
                        text: qsTr("Diagnostics expiry (days)")
                        DescriptiveText.toolTip: description
                        DescriptiveText.activated: diagnosticsExpiryHoverHandler.hovered
                        HoverHandler { id: diagnosticsExpiryHoverHandler }
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    SpinBox {
                        from: 0
                        to: 3650
                        editable: true
                        value: page.pageHandle.diagnosticsExpiryDays
                        onValueModified: page.pageHandle.diagnosticsExpiryDays = value
                        Accessible.description: diagnosticsExpiryLabel.description
                    }
                    Button {
                        Layout.columnSpan: 2
                        text: qsTr("Clear Synthesis Diagnostics")
                        icon.source: "image://fluent-system-icons/delete"
                        onClicked: clearDiagnosticsDialog.open()
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
