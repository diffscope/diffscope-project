// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Internal

import DiffScope.UIShell
import DiffScope.VisualEditor

QtObject {
    id: d

    required property QtObject addOn

    readonly property Component synthesisPieceTrackComponent: FocusScope {
        id: control

        required property PianoRollPanelInterface contextObject

        readonly property QtObject pieceModel: d.addOn?.pieceModel ?? null

        Layout.fillWidth: true
        clip: true
        implicitHeight: 20

        Binding {
            target: control.pieceModel
            property: "singingClip"
            value: control.contextObject?.editingClip ?? null
        }

        TimeManipulator {
            id: timeManipulator

            target: control
            timeLayoutViewModel: control.contextObject?.timeLayoutViewModel ?? null
            timeViewModel: control.contextObject?.timeViewModel ?? null
        }

        Rectangle {
            anchors.fill: parent
            color: Theme.backgroundColor(control.ThemedItem.backgroundLevel)
        }

        Repeater {
            model: control.pieceModel

            delegate: Rectangle {
                id: pieceDelegate

                required property real absolutePosition
                required property real duration
                required property string statusText
                required property string errorMessage
                required property bool ready
                required property bool failed

                readonly property real pixelDensity:
                    control.contextObject?.timeLayoutViewModel?.pixelDensity ?? 0
                readonly property real viewStart:
                    control.contextObject?.timeViewModel?.start ?? 0

                x: (absolutePosition - viewStart) * pixelDensity
                width: duration * pixelDensity
                height: control.height
                visible: width > 0 && x < control.width && x + width > 0
                color: failed
                    ? Theme.errorColor
                    : ready
                        ? Theme.accentColor
                        : Theme.backgroundTertiaryColor
                border.color: Theme.borderColor

                Text {
                    readonly property real boundedWidth:
                        Math.max(0, Math.min(implicitWidth, parent.width - 8))

                    x: Math.max(
                        4,
                        Math.min(
                            (pieceDelegate.viewStart
                                - pieceDelegate.absolutePosition)
                                * pieceDelegate.pixelDensity + 4,
                            parent.width - boundedWidth - 4))
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.max(0, Math.min(implicitWidth, parent.width - x - 4))
                    text: pieceDelegate.statusText
                    color: Theme.foregroundPrimaryColor
                    font: Theme.font
                    elide: Text.ElideRight
                }

                HoverHandler {
                    id: hoverHandler
                }

                ToolTip.visible: hoverHandler.hovered && errorMessage.length > 0
                ToolTip.text: errorMessage
            }
        }

        StandardScrollHandler {
            movableOrientation: Qt.Horizontal
            viewModel: control.contextObject?.scrollBehaviorViewModel ?? null

            onMoved: (x, _, isPhysicalWheel) =>
                timeManipulator.moveViewBy(x, isPhysicalWheel)
            onZoomed: (ratioX, _, x, _, isPhysicalWheel) =>
                timeManipulator.zoomViewBy(ratioX, x, isPhysicalWheel)
        }
    }
}
