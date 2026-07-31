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
                        }

                        Button {
                            text: qsTr("Browse...")
                            TextMatcherItem on text { matcher: page.matcher }
                            onClicked: page.pageHandle.browse()
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Select an existing LibreSVIP command-line executable, or download a compatible build automatically.")
                        TextMatcherItem on text { matcher: page.matcher }
                        wrapMode: Text.Wrap
                    }

                    RowLayout {
                        Button {
                            text: qsTr("Download LibreSVIP")
                            TextMatcherItem on text { matcher: page.matcher }
                            onClicked: page.pageHandle.download()
                        }

                        Button {
                            text: qsTr("Delete downloaded LibreSVIP")
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
