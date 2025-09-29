import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core
import DiffScope.Maintenance

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false
    property bool autoCheckForUpdates: false
    property int updateOption: ApplicationUpdateChecker.UO_Stable

    // Mark dirty when properties change
    onAutoCheckForUpdatesChanged: if (started) pageHandle.markDirty()
    onUpdateOptionChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    contentWidth: availableWidth

    readonly property TextMatcher matcher: TextMatcher {}

    ColumnLayout {
        width: page.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32

            GroupBox {
                title: qsTr("Update Settings")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent

                    CheckBox {
                        text: qsTr("Check for updates on startup")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.autoCheckForUpdates
                        onClicked: page.autoCheckForUpdates = checked
                    }

                    RowLayout {
                        Label {
                            text: qsTr("Type of update to check for")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        ComboBox {
                            model: [
                                qsTr("Stable"),
                                qsTr("Beta"),
                            ]
                            currentIndex: page.updateOption
                            onActivated: (index) => page.updateOption = index
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}