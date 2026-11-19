// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool showPhonemeWaveforms
    property bool showClipWaveforms
    property bool showTimelineStatus
    readonly property TextMatcher matcher: TextMatcher {}

    anchors.fill: parent
    contentWidth: availableWidth

    ColumnLayout {
        width: page.availableWidth

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32

            GroupBox {
                title: qsTr("Waveform")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent

                    CheckBox {
                        text: qsTr("Show waveforms in the phoneme panel")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.showPhonemeWaveforms
                        onToggled: {
                            page.showPhonemeWaveforms = checked
                            page.pageHandle.markDirty()
                        }
                    }

                    CheckBox {
                        text: qsTr("Show waveforms on singing clips")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.showClipWaveforms
                        onToggled: {
                            page.showClipWaveforms = checked
                            page.pageHandle.markDirty()
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Status")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                CheckBox {
                    text: qsTr("Show synthesis status on the timeline")
                    TextMatcherItem on text { matcher: page.matcher }
                    checked: page.showTimelineStatus
                    onToggled: {
                        page.showTimelineStatus = checked
                        page.pageHandle.markDirty()
                    }
                }
            }
        }
    }
}
