// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views

import DiffScope.Synth
import DiffScope.UIShell
import DiffScope.VisualEditor

QtObject {
    id: d

    required property QtObject addOn

    readonly property ProjectViewModelContext projectViewModelContext:
        addOn?.windowHandle.ProjectViewModelContext.context ?? null

    readonly property Component synthesisPieceTrackComponent: RangeIndicatorSequence {
        id: control

        required property PianoRollPanelInterface contextObject

        readonly property SynthesisPieceModel pieceModel:
            d.addOn?.pieceModel ?? null
        readonly property var editingTrackViewModel:
            d.projectViewModelContext?.getTrackViewItemFromDocumentItem(
                control.contextObject?.editingClip?.clipSequence?.track ?? null) ?? null

        property QtObject hoveredPiece: null

        Layout.fillWidth: true
        Theme.accentColor: editingTrackViewModel?.color ?? Qt.rgba(0, 0, 0, 0)

        rangeIndicatorSequenceViewModel:
            pieceModel?.rangeIndicatorSequenceViewModel ?? null
        rangeIndicatorInteractionController:
            d.addOn?.rangeIndicatorInteractionController ?? null
        scrollBehaviorViewModel: contextObject?.scrollBehaviorViewModel ?? null
        timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
        timeViewModel: contextObject?.timeViewModel ?? null

        Binding {
            target: control.pieceModel
            property: "singingClip"
            value: control.contextObject?.editingClip ?? null
        }

        ToolTip.visible:
            control.hoveredPiece !== null
            && control.hoveredPiece.errorMessage.length > 0
        ToolTip.text: control.hoveredPiece?.errorMessage ?? ""

        Connections {
            target: control.rangeIndicatorInteractionController

            function onItemHoverEntered(rangeIndicatorSequence, viewItem) {
                if (rangeIndicatorSequence !== control)
                    return
                control.hoveredPiece =
                    control.pieceModel?.pieceForRangeIndicator(viewItem) ?? null
            }

            function onItemHoverExited(rangeIndicatorSequence, viewItem) {
                if (rangeIndicatorSequence !== control)
                    return
                const piece =
                    control.pieceModel?.pieceForRangeIndicator(viewItem) ?? null
                if (control.hoveredPiece === piece)
                    control.hoveredPiece = null
            }

            function onItemContextMenuRequested(rangeIndicatorSequence, viewItem) {
                if (rangeIndicatorSequence !== control)
                    return
                const piece =
                    control.pieceModel?.pieceForRangeIndicator(viewItem) ?? null
                if (piece === null)
                    return
                pieceMenu.piece = piece
                pieceMenu.canTerminate =
                    control.pieceModel?.isPieceTaskActive(piece) ?? false
                pieceMenu.pieceDiagnosticFilePath =
                    control.pieceModel?.verifiedDiagnosticFilePath(piece) ?? ""
                pieceMenu.popup()
                pieceMenu.anchorX = pieceMenu.x
                pieceMenu.anchorY = pieceMenu.y
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
                enabled: pieceMenu.canTerminate && pieceMenu.piece !== null
                onTriggered: control.pieceModel.cancelPieceTask(pieceMenu.piece)
            }

            Action {
                text: qsTr("View Diagnostics")
                icon.source: "image://fluent-system-icons/document_search"
                enabled: pieceMenu.pieceDiagnosticFilePath.length > 0
                onTriggered: DesktopServices.reveal(
                    pieceMenu.pieceDiagnosticFilePath)
            }
        }

        Component {
            id: resynthesizeDialogComponent

            ResynthesizeDialog {
            }
        }

        function openResynthesizeDialog() {
            const piece = pieceMenu.piece
            if (piece === null)
                return
            const window = control.Window.window
            if (window === null)
                return
            const dialog = resynthesizeDialogComponent.createObject(
                window.contentItem, {
                    resynthesizeFrom: 0,
                    disableCache: false,
                })
            if (dialog === null)
                return
            const width = dialog.width
            const height = dialog.height
            dialog.x = Math.max(
                0, Math.min(pieceMenu.anchorX, window.width - width))
            dialog.y = Math.max(
                0, Math.min(pieceMenu.anchorY, window.height - height))
            dialog.accepted.connect(() => {
                control.pieceModel.resynthesizePiece(
                    piece, dialog.resynthesizeFrom,
                    !dialog.disableCache, true)
                dialog.destroy()
            })
            dialog.rejected.connect(() => dialog.destroy())
            dialog.open()
        }
    }
}
