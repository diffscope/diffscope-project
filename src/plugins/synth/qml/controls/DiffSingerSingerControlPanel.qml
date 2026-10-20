// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import "JsonState.js" as JsonState

ScrollView {
    id: control

    contentWidth: availableWidth

    signal stateEdited()

    QtObject {
        id: d

        property var speakers: []
        property var currentState: ({})
        readonly property string selectedSpeaker: currentState.speaker ?? speakers[0]?.id ?? ""
    }

    ButtonGroup {
        id: speakerGroup
    }

    function validateInfo(info) {
        if (!info || typeof info !== "object" || Array.isArray(info) || !Array.isArray(info.speakers))
            return false
        const ids = new Set()
        for (const speaker of info.speakers) {
            if (!speaker || typeof speaker !== "object" || Array.isArray(speaker)
                    || typeof speaker.id !== "string" || speaker.id.length === 0
                    || typeof speaker.name !== "string" || ids.has(speaker.id))
                return false
            ids.add(speaker.id)
        }
        return true
    }

    function loadInfo(info) {
        if (!validateInfo(info))
            return false
        d.speakers = JsonState.copy(info.speakers)
    }

    function validateState(state) {
        if (!state || typeof state !== "object" || Array.isArray(state))
            return false
        if (state.speaker === undefined)
            return true
        if (typeof state.speaker !== "string")
            return false
        return d.speakers.length === 0 ? state.speaker === ""
                                     : d.speakers.some(speaker => speaker.id === state.speaker)
    }

    function loadState(state) {
        if (!validateState(state))
            return false
        d.currentState = JsonState.copy(state)
    }

    function saveState() {
        return JsonState.copy(d.currentState)
    }

    function selectSpeaker(id) {
        if (d.selectedSpeaker === id)
            return
        const state = JsonState.copy(d.currentState)
        state.speaker = id
        d.currentState = state
        stateEdited()
    }

    ColumnLayout {
        width: control.availableWidth

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Speaker")

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Repeater {
                    model: d.speakers.length > 0 ? d.speakers : [{id: "", name: qsTr("Default Speaker")}]

                    delegate: RadioButton {
                        id: speakerOption
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        padding: 12
                        topPadding: padding
                        bottomPadding: padding
                        spacing: 12
                        hoverEnabled: false
                        text: modelData.name
                        checked: d.speakers.length === 0 || modelData.id === d.selectedSpeaker
                        ButtonGroup.group: speakerGroup
                        onClicked: control.selectSpeaker(modelData.id)

                        HoverHandler {
                            id: cardHover
                        }

                        background: Rectangle {
                            radius: 4
                            color: speakerOption.down ? Theme.controlPressedColorChange.apply(Theme.backgroundSecondaryColor)
                                                     : cardHover.hovered ? Theme.controlHoveredColorChange.apply(Theme.backgroundSecondaryColor)
                                                                         : Theme.backgroundSecondaryColor
                            border.width: 1
                            border.color: speakerOption.checked ? Theme.accentColor : Theme.borderColor

                            Behavior on color {
                                ColorAnimation {
                                    duration: Theme.colorAnimationDuration
                                    easing.type: Easing.OutCubic
                                }
                            }

                            Behavior on border.color {
                                ColorAnimation {
                                    duration: Theme.colorAnimationDuration
                                    easing.type: Easing.OutCubic
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 3
                                color: "transparent"
                                radius: 2
                                border.width: 2
                                border.color: Theme.navigationColor
                                visible: speakerOption.visualFocus
                            }
                        }

                        contentItem: Label {
                            text: speakerOption.text
                            wrapMode: Text.Wrap
                            leftPadding: speakerOption.mirrored ? 0 : speakerOption.indicator.width + speakerOption.spacing
                            rightPadding: speakerOption.mirrored ? speakerOption.indicator.width + speakerOption.spacing : 0
                        }
                    }
                }
            }
        }
    }
}
