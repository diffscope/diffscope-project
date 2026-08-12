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
                        onClicked: page.pageHandle.clearCache()
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
