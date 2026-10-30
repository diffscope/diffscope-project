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

    required property ArrangementPanelInterface contextObject

    readonly property ProjectViewModelContext projectViewModelContext: contextObject?.windowHandle.ProjectViewModelContext.context ?? null
    readonly property AudioFileDropAddOn addOn: contextObject?.windowHandle.AudioFileDropAddOn.addOn ?? null
    readonly property TimeLayoutViewModel timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
    property bool cursorPositionActive: false
    property point pointerPosition: Qt.point(0, 0)
    property int targetPosition: 0

    anchors.fill: parent
    z: 1
    enabled: !(addOn?.importing ?? true)

    function updateCursorPosition() {
        if (!cursorPositionActive)
            return
        targetPosition = Math.max(0, timeManipulator.alignPosition(timeManipulator.mapToPosition(pointerPosition.x), ScopicFlow.AO_Visible))
        if (timeLayoutViewModel)
            timeLayoutViewModel.cursorPosition = targetPosition
    }

    function finishDrag() {
        if (dragScroller)
            dragScroller.running = false
        if (cursorPositionActive) {
            cursorPositionActive = false
            if (timeLayoutViewModel)
                timeLayoutViewModel.cursorPosition = -1
        }
    }

    function updateDrag(drag) {
        drag.accepted = false
        if (!drag.hasUrls || !(drag.supportedActions & Qt.CopyAction) || !addOn?.acceptsUrls(drag.urls)) {
            finishDrag()
            return
        }
        drag.accept(Qt.CopyAction)
        cursorPositionActive = true
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
        timeViewModel: dropArea.contextObject?.timeViewModel ?? null
        timeLayoutViewModel: dropArea.timeLayoutViewModel
    }

    TrackListManipulator {
        id: trackListManipulator
        target: dropArea
        trackListViewModel: dropArea.projectViewModelContext?.trackListViewModel ?? null
        trackListLayoutViewModel: dropArea.contextObject?.trackListLayoutViewModel ?? null
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
        target: dropArea.contextObject?.timeViewModel ?? null
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
            cursorPositionActive = true
            pointerPosition = Qt.point(drop.x, drop.y)
            updateCursorPosition()
            const index = trackListManipulator.mapToPosition(drop.y)
            const trackViewModel = projectViewModelContext.trackListViewModel.items[index]
            const track = trackViewModel ? projectViewModelContext.getTrackDocumentItemFromViewItem(trackViewModel) : null
            if (addOn.importFiles(drop.urls, track, targetPosition))
                drop.accept(Qt.CopyAction)
        }
        finishDrag()
    }

    Component.onDestruction: finishDrag()
}
