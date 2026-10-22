// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false
    property bool autoCheckForUpdates: true
    readonly property TextMatcher matcher: TextMatcher {}

    onAutoCheckForUpdatesChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    contentWidth: availableWidth

    ColumnLayout {
        width: page.availableWidth

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
                title: qsTr("LibreSVIP Command-Line Tool")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            id: installationLabel
                            text: qsTr("Installation")
                            TextMatcherItem on text { matcher: page.matcher }
                        }

                        ComboBox {
                            id: installationCombo
                            Layout.fillWidth: true
                            model: page.pageHandle.installations
                            textRole: "name"
                            valueRole: "id"
                            currentValue: page.pageHandle.installationId
                            Accessible.name: installationLabel.text
                            onActivated: {
                                if (!page.pageHandle.selectInstallation(currentValue))
                                    currentIndex = indexOfValue(page.pageHandle.installationId)
                            }
                            delegate: ItemDelegate {
                                id: installationDelegate
                                required property int index
                                required property var modelData
                                width: installationCombo.width
                                highlighted: installationCombo.highlightedIndex === index
                                text: modelData.name
                                Accessible.description: modelData.path
                                contentItem: ColumnLayout {
                                    spacing: 2
                                    Label {
                                        Layout.fillWidth: true
                                        text: installationDelegate.modelData.name
                                        textFormat: Text.PlainText
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        visible: text.length > 0
                                        text: installationDelegate.modelData.path
                                        textFormat: Text.PlainText
                                        color: Theme.foregroundSecondaryColor
                                        font.pixelSize: 11
                                        elide: Text.ElideMiddle
                                    }
                                }
                            }
                        }

                        ToolButton {
                            text: qsTr("Add Local Installation...")
                            icon.source: "image://fluent-system-icons/add"
                            display: AbstractButton.IconOnly
                            onClicked: page.pageHandle.browse()
                        }

                        ToolButton {
                            text: qsTr("Remove Local Installation from List")
                            icon.source: "image://fluent-system-icons/subtract"
                            display: AbstractButton.IconOnly
                            enabled: page.pageHandle.installationId.startsWith("local:")
                            onClicked: page.pageHandle.removeLocalInstallation()
                        }
                    }

                    TextField {
                        Layout.fillWidth: true
                        readOnly: true
                        selectByMouse: true
                        text: page.pageHandle.executablePath
                        placeholderText: qsTr("No LibreSVIP installation is selected")
                        Accessible.name: qsTr("Executable path")
                        TextMatcherItem on placeholderText { matcher: page.matcher }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        Button {
                            text: qsTr("Download LibreSVIP...")
                            icon.source: "image://fluent-system-icons/arrow_download"
                            TextMatcherItem on text { matcher: page.matcher }
                            onClicked: page.pageHandle.download()
                        }

                        Button {
                            text: qsTr("Uninstall Downloaded Version")
                            icon.source: "image://fluent-system-icons/delete"
                            TextMatcherItem on text { matcher: page.matcher }
                            enabled: page.pageHandle.selectedInstallationIsDownloaded
                            onClicked: page.pageHandle.uninstallSelectedInstallation()
                        }
                    }

                    CheckBox {
                        text: qsTr("Automatically check for LibreSVIP updates")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.autoCheckForUpdates
                        onClicked: page.autoCheckForUpdates = checked
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
