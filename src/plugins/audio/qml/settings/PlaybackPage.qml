import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Audio

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false

    property int playbackBehavior
    property int playbackTogglingAction

    readonly property var playbackBehaviorModel: [
        {text: qsTr("Return to start position"), value: AudioPreference.PB_ReturnToStart},
        {text: qsTr("Keep at current position"), value: AudioPreference.PB_KeepAtCurrent},
        {text: qsTr("Keep at current position, but play from start position next time"), value: AudioPreference.PB_KeepAtCurrentButPlayFromStart},
    ]
    readonly property var playbackTogglingActionModel: [
        {text: qsTr("Play/Stop"), value: AudioPreference.PTA_PlayStop},
        {text: qsTr("Play/Pause"), value: AudioPreference.PTA_PlayPause},
    ]

    onPlaybackBehaviorChanged: if (started) pageHandle.markDirty()
    onPlaybackTogglingActionChanged: if (started) pageHandle.markDirty()

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
                title: qsTr("Playback")
                TextMatcherItem on title {
                    matcher: page.matcher
                }
                Layout.fillWidth: true
                GridLayout {
                    columns: 2
                    anchors.fill: parent
                    Label {
                        Layout.columnSpan: 2
                        text: qsTr("When playback stops, make playhead")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    ComboBox {
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        textRole: "text"
                        valueRole: "value"
                        model: page.playbackBehaviorModel
                        currentIndex: page.playbackBehavior
                        onCurrentValueChanged: page.playbackBehavior = currentValue
                    }
                    Label {
                        text: qsTr("Playback toggling action")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        textRole: "text"
                        valueRole: "value"
                        model: page.playbackTogglingActionModel
                        currentIndex: page.playbackTogglingAction
                        onCurrentValueChanged: page.playbackTogglingAction = currentValue
                    }
                }
            }
        }
    }
}
