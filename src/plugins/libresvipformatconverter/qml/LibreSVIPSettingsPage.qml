import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

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
            spacing: 24

            RowLayout {
                Layout.fillWidth: true
                Image {
                    source: "qrc:/libresvipformatconverter/res/libresvip/libresvip.ico"
                    Layout.preferredWidth: 96
                    Layout.preferredHeight: 96
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("LibreSVIP")
                        font.pixelSize: 16
                        wrapMode: Text.Wrap
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("A universal converter for singing-voice-synthesis projects")
                        wrapMode: Text.Wrap
                    }
                    LinkLabel {
                        href: page.pageHandle.homepageUrl
                        linkText: qsTr("Visit homepage")
                        externalLink: true
                        onLinkActivated: (link) => Qt.openUrlExternally(link)
                    }
                }
            }

            GroupBox {
                title: qsTr("LibreSVIP command-line tool")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        TextField {
                            Layout.fillWidth: true
                            readOnly: true
                            text: page.pageHandle.executablePath
                            placeholderText: qsTr("No LibreSVIP executable is configured")
                            TextMatcherItem on placeholderText { matcher: page.matcher }
                            selectByMouse: true
                            rightPadding: 32
                            ToolButton {
                                anchors.right: parent.right
                                anchors.rightMargin: 2
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("Clear")
                                icon.source: "image://fluent-system-icons/dismiss_circle"
                                display: AbstractButton.IconOnly
                                visible: page.pageHandle.executablePath.length > 0
                                onClicked: page.pageHandle.clearExecutablePath()
                            }
                        }

                        ToolButton {
                            text: qsTr("Browse...")
                            icon.source: "image://fluent-system-icons/folder_open"
                            display: AbstractButton.IconOnly
                            onClicked: page.pageHandle.browse()
                        }
                    }

                    RowLayout {
                        Button {
                            text: qsTr("Download LibreSVIP...")
                            icon.source: "image://fluent-system-icons/arrow_download"
                            TextMatcherItem on text { matcher: page.matcher }
                            onClicked: page.pageHandle.download()
                        }

                        Button {
                            text: qsTr("Uninstall Downloaded LibreSVIP")
                            icon.source: "image://fluent-system-icons/delete"
                            TextMatcherItem on text { matcher: page.matcher }
                            enabled: page.pageHandle.downloadedInstallationExists
                            onClicked: page.pageHandle.removeDownloadedInstallation()
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
