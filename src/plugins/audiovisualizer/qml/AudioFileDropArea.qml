// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Internal as ScopicFlowInternal

import DiffScope.AudioVisualizer
import DiffScope.VisualEditor

DropArea {
    id: dropArea

    readonly property ArrangementPanelInterface arrangementPanelInterface: ViewLayer.arrangementPanelInterface

    readonly property ProjectViewModelContext projectViewModelContext: arrangementPanelInterface?.windowHandle.ProjectViewModelContext.context ?? null
    readonly property AudioFileDropAddOn addOn: arrangementPanelInterface?.windowHandle.AudioFileDropAddOn.addOn ?? null
    readonly property TimeLayoutViewModel timeLayoutViewModel: arrangementPanelInterface?.timeLayoutViewModel ?? null
    property bool timeCursorActive: false
    property point pointerPosition: Qt.point(0, 0)
    property int timeCursorPosition: 0

    anchors.fill: parent
    z: 1
    enabled: !(addOn?.importing ?? true)

    function updateCursorPosition() {
        if (!timeCursorActive)
            return
        timeCursorPosition = Math.max(0, timeManipulator.alignPosition(timeManipulator.mapToPosition(pointerPosition.x), ScopicFlow.AO_Visible))
    }

    function finishDrag() {
        if (dragScroller)
            dragScroller.running = false
        timeCursorActive = false
    }

    function updateDrag(drag) {
        drag.accepted = false
        if (!drag.hasUrls || !(drag.supportedActions & Qt.CopyAction) || !addOn?.acceptsUrls(drag.urls)) {
            finishDrag()
            return
        }
        drag.accept(Qt.CopyAction)
        timeCursorActive = true
        pointerPosition = Qt.point(drag.x, drag.y)
        updateCursorPosition()
        const marginX = Math.min(32, width / 2)
        const marginY = Math.min(32, height / 2)
        dragScroller.determine(drag.x - marginX, width - 2 * marginX,
                               drag.y - marginY, height - 2 * marginY, () => {})
    }

    TimeManipulator {
        id: timeManipulator
        target: dropArea
        timeViewModel: dropArea.arrangementPanelInterface?.timeViewModel ?? null
        timeLayoutViewModel: dropArea.timeLayoutViewModel
    }

    TrackListManipulator {
        id: trackListManipulator
        target: dropArea
        trackListViewModel: dropArea.projectViewModelContext?.trackListViewModel ?? null
        trackListLayoutViewModel: dropArea.arrangementPanelInterface?.trackListLayoutViewModel ?? null
    }

    ScopicFlowInternal.DragScroller {
        id: dragScroller
        onMoved: (deltaX, deltaY) => {
            timeManipulator.moveViewBy(deltaX)
            trackListManipulator.moveViewBy(deltaY)
            dropArea.updateCursorPosition()
        }
    }

    Connections {
        target: dropArea.arrangementPanelInterface?.timeViewModel ?? null
        function onStartChanged() {
            dropArea.updateCursorPosition()
        }
    }

    Connections {
        target: dropArea.timeLayoutViewModel
        function onPositionAlignmentChanged() {
            dropArea.updateCursorPosition()
        }
        function onPixelDensityChanged() {
            dropArea.updateCursorPosition()
        }
    }

    onEntered: drag => updateDrag(drag)
    onPositionChanged: drag => updateDrag(drag)
    onExited: finishDrag()
    onEnabledChanged: {
        if (!enabled)
            finishDrag()
    }
    onVisibleChanged: {
        if (!visible)
            finishDrag()
    }
    onDropped: drop => {
        drop.accepted = false
        if (drop.hasUrls && (drop.supportedActions & Qt.CopyAction) && addOn?.acceptsUrls(drop.urls)) {
            timeCursorActive = true
            pointerPosition = Qt.point(drop.x, drop.y)
            updateCursorPosition()
            const index = trackListManipulator.mapToPosition(drop.y)
            const trackViewModel = projectViewModelContext.trackListViewModel.items[index]
            const track = trackViewModel ? projectViewModelContext.getTrackDocumentItemFromViewItem(trackViewModel) : null
            if (addOn.importFiles(drop.urls, track, timeCursorPosition))
                drop.accept(Qt.CopyAction)
        }
        finishDrag()
    }

    Component.onDestruction: finishDrag()
}
