// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

import dev.sjimo.ScopicFlow

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.VisualEditor

Loader {
    id: control

    readonly property PianoRollPanelInterface panelInterface: ViewLayer.pianoRollPanelInterface

    z: -1
    active: SynthesisVisualizerSettings.showTimelineStatus

    sourceComponent: Item {
        id: statusLayer

        readonly property ProjectViewModelContext projectViewModelContext:
            control.ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
        readonly property TrackViewModel editingTrackViewModel:
            projectViewModelContext?.getTrackViewItemFromDocumentItem(
                control.panelInterface?.editingClip?.clipSequence?.track ?? null) ?? null

        Theme.accentColor: editingTrackViewModel?.color ?? control.Theme.accentColor

        SynthesisPieceModel {
            id: pieceModel

            windowHandle: control.ViewLayer.windowHandle
            singingClip: control.panelInterface?.editingClip ?? null
        }

        Repeater {
            model: pieceModel

            delegate: Rectangle {
                id: status

                required property real absolutePosition
                required property real duration
                required property bool active
                required property bool ready
                required property bool failed

                readonly property real startTick: Math.sign(absolutePosition) * Math.round(Math.abs(absolutePosition))

                anchors.bottom: parent.bottom
                x: control.ViewLayer.timeViewport.xForTick(startTick)
                width: control.ViewLayer.timeViewport.xForTick(startTick + Math.round(duration)) - x
                height: 2
                visible: width > 0 && x < statusLayer.width && x + width > 0
                color: ready ? Theme.accentColor : failed ? Theme.errorColor : Theme.controlColor(SVS.CT_Normal)

                Rectangle {
                    id: pulseOverlay

                    readonly property int animationDuration: Theme.visualEffectAnimationDuration * 2

                    anchors.fill: parent
                    color: Theme.foregroundSecondaryColor
                    opacity: 0
                    visible: status.active && animationDuration > 0

                    SequentialAnimation {
                        running: pulseOverlay.visible
                        loops: Animation.Infinite
                        onStopped: pulseOverlay.opacity = 0

                        NumberAnimation {
                            target: pulseOverlay
                            property: "opacity"
                            from: 0
                            to: 1
                            duration: pulseOverlay.animationDuration
                            easing.type: Easing.InOutCubic
                        }

                        NumberAnimation {
                            target: pulseOverlay
                            property: "opacity"
                            from: 1
                            to: 0
                            duration: pulseOverlay.animationDuration
                            easing.type: Easing.InOutCubic
                        }
                    }
                }
            }
        }
    }
}
