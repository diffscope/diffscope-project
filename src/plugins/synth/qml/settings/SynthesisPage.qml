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
                        text: qsTr("Base piece padding (ms)")
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
                    }
                    Label {
                        text: qsTr("Additional onset padding (ms)")
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
                    }
                    Label {
                        text: qsTr("Piece gap threshold (ms)")
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
                    }
                    Label {
                        text: qsTr("Rest lyrics")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: page.pageHandle.restLyrics
                        placeholderText: qsTr("Comma-separated, for example AP, SP")
                        onTextEdited: page.pageHandle.restLyrics = text
                    }
                    Label {
                        text: qsTr("Parameter sample rate (Hz)")
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
                    }
                    Label {
                        text: qsTr("Singer mix sample rate (Hz)")
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
                        text: qsTr("Maximum cache size (GiB)")
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
                    }
                    Label {
                        text: qsTr("Cache expiry (days)")
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
                    }
                    Label {
                        text: qsTr("Maximum audio download (MiB)")
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
                    }
                    Label {
                        text: qsTr("Environment lifetime (seconds)")
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
                    }
                    Button {
                        Layout.columnSpan: 2
                        text: qsTr("Clear Synthesis Cache")
                        icon.source: "image://fluent-system-icons/delete"
                        onClicked: clearCacheDialog.open()
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
