// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

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
    property string fallbackLyricText
    property bool skipParameterResamplingCheck

    onFallbackLyricTextChanged: if (started) pageHandle.markDirty()
    onSkipParameterResamplingCheckChanged: if (started) pageHandle.markDirty()

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
                title: qsTr("Notes")
                Layout.fillWidth: true
                TextMatcherItem on title { matcher: page.matcher }
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    Label {
                        id: fallbackLyricLabel
                        text: qsTr("Fallback lyric text")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: page.fallbackLyricText
                        onTextEdited: page.fallbackLyricText = text
                        Accessible.name: fallbackLyricLabel.text
                    }
                }
            }
            GroupBox {
                title: qsTr("Parameters")
                Layout.fillWidth: true
                TextMatcherItem on title { matcher: page.matcher }
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Skip parameter resampling checks")
                        checked: page.skipParameterResamplingCheck
                        onClicked: page.skipParameterResamplingCheck = checked
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        text: qsTr("Parameter curves not aligned to step size will be resampled automatically when pasting or bouncing clips.")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                }
            }
        }
    }
}
