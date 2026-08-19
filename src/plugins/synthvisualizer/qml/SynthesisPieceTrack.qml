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

import DiffScope.Synth
import DiffScope.UIShell
import DiffScope.VisualEditor

QtObject {
    id: d

    required property QtObject addOn

    readonly property ProjectViewModelContext projectViewModelContext:
        addOn?.windowHandle.ProjectViewModelContext.context ?? null

    readonly property Component synthesisPieceTrackComponent: FocusScope {
        id: control

        required property PianoRollPanelInterface contextObject

        readonly property QtObject pieceModel: d.addOn?.pieceModel ?? null
        readonly property var editingTrackViewModel:
            d.projectViewModelContext?.getTrackViewItemFromDocumentItem(
                control.contextObject?.editingClip?.clipSequence?.track ?? null) ?? null
        readonly property color trackColor:
            editingTrackViewModel?.color ?? Theme.accentColor

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
                required property bool active
                required property QtObject piece
                required property string diagnosticFilePath

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
                        ? control.trackColor
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

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        const window = control.Window.window
                        const anchor = window
                            ? pieceDelegate.mapToItem(window.contentItem, mouse.x, mouse.y)
                            : Qt.point(mouse.x, mouse.y)
                        pieceMenu.piece = pieceDelegate.piece
                        pieceMenu.canTerminate = pieceDelegate.active
                        pieceMenu.pieceDiagnosticFilePath = pieceDelegate.diagnosticFilePath
                        pieceMenu.anchorX = anchor.x
                        pieceMenu.anchorY = anchor.y
                        pieceMenu.popup(pieceDelegate, mouse.x, mouse.y)
                    }
                }

                HoverHandler {
                    id: hoverHandler
                }

                ToolTip.visible: hoverHandler.hovered && errorMessage.length > 0
                ToolTip.text: errorMessage
            }
        }

        Menu {
            id: pieceMenu

            property QtObject piece: null
            property bool canTerminate: false
            property string pieceDiagnosticFilePath: ""
            property double anchorX: 0
            property double anchorY: 0

            Action {
                text: qsTr("Resynthesize...")
                icon.source: "image://fluent-system-icons/arrow_sync"
                onTriggered: control.openResynthesizeDialog()
            }
            Action {
                text: qsTr("Terminate Task")
                icon.source: "image://fluent-system-icons/stop"
                enabled: pieceMenu.canTerminate && pieceMenu.piece != null
                onTriggered: control.pieceModel.cancelPieceTask(pieceMenu.piece)
            }
            Action {
                text: qsTr("View Diagnostics")
                icon.source: "image://fluent-system-icons/document_search"
                enabled: pieceMenu.pieceDiagnosticFilePath.length > 0
                onTriggered: DesktopServices.reveal(pieceMenu.pieceDiagnosticFilePath)
            }
        }

        Component {
            id: resynthesizeDialogComponent
            ResynthesizeDialog {}
        }

        function openResynthesizeDialog() {
            const piece = pieceMenu.piece
            if (piece == null)
                return
            const window = control.Window.window
            if (window == null)
                return
            const dialog = resynthesizeDialogComponent.createObject(window.contentItem, {
                resynthesizeFrom: 0,
                disableCache: false,
            })
            if (dialog == null)
                return
            const width = dialog.width
            const height = dialog.height
            dialog.x = Math.max(0, Math.min(pieceMenu.anchorX, window.width - width))
            dialog.y = Math.max(0, Math.min(pieceMenu.anchorY, window.height - height))
            dialog.accepted.connect(() => {
                control.pieceModel.resynthesizePiece(
                    piece, dialog.resynthesizeFrom, !dialog.disableCache, true)
                dialog.destroy()
            })
            dialog.rejected.connect(() => dialog.destroy())
            dialog.open()
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
