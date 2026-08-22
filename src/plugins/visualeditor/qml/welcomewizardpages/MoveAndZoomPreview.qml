// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Internal

import DiffScope.VisualEditor

Item {
    id: preview

    clip: true

    readonly property double baseCellSize: 96
    // A new detail level fades in over a 16x interval span: its lines gradually
    // emerge from transparent while they are spaced between
    // baseCellSize / 2^lodFadeOctaves and baseCellSize pixels apart.
    readonly property int lodFadeOctaves: 4
    // The grid has unbounded detail levels. On screen, level k lines are spaced
    // baseCellSize * 2^(k + zoomLevel) pixels apart and fade in gradually while
    // zooming in, so no visual discontinuity ever occurs.

    property double startX: 0
    property double startY: 0
    property double zoomLevelX: 0
    property double zoomLevelY: 0
    property bool animated: false

    function scrollModifierValue(scrollModifier) {
        switch (scrollModifier) {
        case EditorPreference.SM_Control:
            return Qt.ControlModifier
        case EditorPreference.SM_Alt:
            return Qt.AltModifier
        case EditorPreference.SM_Shift:
        default:
            return Qt.ShiftModifier
        }
    }

    function moveViewBy(deltaX, deltaY, wheelAnimated) {
        animated = wheelAnimated
        startX += deltaX / (baseCellSize * Math.pow(2, zoomLevelX))
        startY += deltaY / (baseCellSize * Math.pow(2, zoomLevelY))
    }

    function zoomViewBy(ratioX, ratioY, x, y, wheelAnimated) {
        animated = wheelAnimated
        if (ratioX !== 1) {
            let anchor = startX + x / (baseCellSize * Math.pow(2, zoomLevelX))
            let target = zoomLevelX + Math.log2(ratioX)
            zoomLevelX = target
            startX = anchor - x / (baseCellSize * Math.pow(2, target))
        }
        if (ratioY !== 1) {
            let anchor = startY + y / (baseCellSize * Math.pow(2, zoomLevelY))
            let target = zoomLevelY + Math.log2(ratioY)
            zoomLevelY = target
            startY = anchor - y / (baseCellSize * Math.pow(2, target))
        }
    }

    ScrollBehaviorViewModel {
        id: scrollBehaviorViewModel

        alternateAxisModifier: preview.scrollModifierValue(EditorPreference.alternateAxisModifier)
        zoomModifier: preview.scrollModifierValue(EditorPreference.zoomModifier)
        pageModifier: preview.scrollModifierValue(EditorPreference.pageModifier)
        usePageModifierAsAlternateAxisZoom: EditorPreference.usePageModifierAsAlternateAxisZoom
        autoScroll: EditorPreference.middleButtonAutoScroll
    }

    Canvas {
        id: canvas

        anchors.fill: parent

        readonly property color backgroundColor: SFPalette.editAreaPrimaryColor
        readonly property color lineColor: SFPalette.scaleSecondaryColor
        property double startX: preview.startX
        property double startY: preview.startY
        property double zoomLevelX: preview.zoomLevelX
        property double zoomLevelY: preview.zoomLevelY

        onBackgroundColorChanged: requestPaint()
        onLineColorChanged: requestPaint()
        onStartXChanged: requestPaint()
        onStartYChanged: requestPaint()
        onZoomLevelXChanged: requestPaint()
        onZoomLevelYChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        function drawGridLines(ctx, vertical) {
            let zoomLevel = vertical ? zoomLevelX : zoomLevelY
            let start = vertical ? startX : startY
            let extent = vertical ? width : height
            let base = preview.baseCellSize
            for (let level = Math.ceil(-preview.lodFadeOctaves - zoomLevel); ; level++) {
                let interval = base * Math.pow(2, level + zoomLevel)
                if (interval > extent * 2)
                    break
                ctx.globalAlpha = Math.max(0, Math.min(1, (level + zoomLevel + preview.lodFadeOctaves) / preview.lodFadeOctaves))
                let cellSpan = Math.pow(2, level)
                for (let m = Math.ceil(start / cellSpan), x = (m * cellSpan - start) * base * Math.pow(2, zoomLevel); x < extent; m++, x += interval) {
                    if (vertical)
                        ctx.fillRect(x, 0, 1, height)
                    else
                        ctx.fillRect(0, x, width, 1)
                }
            }
            ctx.globalAlpha = 1
        }

        onPaint: function (region) {
            let ctx = getContext("2d")
            ctx.fillStyle = backgroundColor
            ctx.fillRect(0, 0, width, height)
            ctx.fillStyle = lineColor
            drawGridLines(ctx, true)
            drawGridLines(ctx, false)
        }
    }

    StandardScrollHandler {
        viewModel: scrollBehaviorViewModel
        movableOrientation: Qt.Horizontal | Qt.Vertical
        zoomableOrientation: Qt.Horizontal | Qt.Vertical

        onMoved: (deltaX, deltaY, isPhysicalWheel) => preview.moveViewBy(deltaX, deltaY, isPhysicalWheel)
        onZoomed: (ratioX, ratioY, x, y, isPhysicalWheel) => preview.zoomViewBy(ratioX, ratioY, x, y, isPhysicalWheel)
    }

    Behavior on startX {
        enabled: preview.animated
        NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
    }
    Behavior on startY {
        enabled: preview.animated
        NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
    }
    Behavior on zoomLevelX {
        enabled: preview.animated
        NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
    }
    Behavior on zoomLevelY {
        enabled: preview.animated
        NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
    }
}
