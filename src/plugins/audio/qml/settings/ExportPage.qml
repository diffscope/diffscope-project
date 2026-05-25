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

    property bool audioExporterClippingCheckEnabled
    property bool audioExporterUseTemporaryFile
    property bool shouldPlayNotificationSoundWhenExportCompleted

    onAudioExporterClippingCheckEnabledChanged: if (started) pageHandle.markDirty()
    onAudioExporterUseTemporaryFileChanged: if (started) pageHandle.markDirty()
    onShouldPlayNotificationSoundWhenExportCompletedChanged: if (started) pageHandle.markDirty()

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
                title: qsTr("Export")
                TextMatcherItem on title {
                    matcher: page.matcher
                }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Check clipping when exporting audio")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                        checked: page.audioExporterClippingCheckEnabled
                        onClicked: page.audioExporterClippingCheckEnabled = checked
                    }
                    CheckBox {
                        text: qsTr("Use temporary file when exporting audio")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                        checked: page.audioExporterUseTemporaryFile
                        onClicked: page.audioExporterUseTemporaryFile = checked
                    }
                    CheckBox {
                        text: qsTr("Play notification sound when export completes")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                        checked: page.shouldPlayNotificationSoundWhenExportCompleted
                        onClicked: page.shouldPlayNotificationSoundWhenExportCompleted = checked
                    }
                }
            }
        }
    }
}
