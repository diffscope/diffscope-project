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
    required property var architectureExtraModel
    readonly property TextMatcher matcher: TextMatcher {}
    contentWidth: availableWidth

    ColumnLayout {
        width: parent.width
        spacing: 12

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            visible: text.length > 0
            text: page.pageHandle.errorMessage
            color: Theme.errorColor
            wrapMode: Text.Wrap
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.margins: 12
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

        Label {
            Layout.leftMargin: 12
            text: qsTr("Cache")
            font.weight: Font.DemiBold
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            implicitHeight: 1
            color: Theme.borderColor
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
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
                text: qsTr("Environment tag lifetime (seconds)")
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
                onClicked: page.pageHandle.clearCache()
            }
        }

        Label {
            Layout.leftMargin: 12
            text: qsTr("Architecture Extra Values")
            font.weight: Font.DemiBold
        }
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            text: qsTr("Each value is sent as arch_extra. Architectures without an entry receive null.")
            wrapMode: Text.Wrap
            ThemedItem.foregroundLevel: SVS.FL_Secondary
        }

        ListView {
            id: extras
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            implicitHeight: Math.max(160, contentHeight)
            model: page.architectureExtraModel
            spacing: 8
            clip: true

            delegate: Frame {
                id: extraDelegate
                required property var model
                required property int index
                width: ListView.view.width
                implicitHeight: extraContent.implicitHeight + topPadding + bottomPadding

                ColumnLayout {
                    id: extraContent
                    width: parent.width
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            Layout.fillWidth: true
                            text: extraDelegate.model.architectureId
                            placeholderText: qsTr("Architecture id")
                            onTextEdited: extraDelegate.model.architectureId = text
                        }
                        ToolButton {
                            text: qsTr("Delete")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/delete"
                            onClicked: page.architectureExtraModel.removeEntry(extraDelegate.index)
                        }
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        implicitHeight: 100
                        TextArea {
                            text: extraDelegate.model.json
                            placeholderText: qsTr("Raw JSON value")
                            wrapMode: TextEdit.NoWrap
                            onTextChanged: if (activeFocus)
                                extraDelegate.model.json = text
                        }
                    }
                }
            }
        }

        Button {
            Layout.leftMargin: 12
            Layout.bottomMargin: 12
            text: qsTr("Add Architecture")
            icon.source: "image://fluent-system-icons/add"
            onClicked: page.architectureExtraModel.addEntry()
        }
    }
}
