// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import dev.sjimo.ScopicFlow

import SVSCraft.UIComponents

Item {
    id: layer

    readonly property ProjectViewModelContext context: ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
    readonly property rect viewport: ViewLayer.visibleRect
    readonly property ClipViewModel clipViewModel: ViewLayer.clipViewModel
    readonly property ClipPaneInteractionController clipPaneInteractionController:
        ViewLayer.arrangementPanelInterface?.clipPaneInteractionController ?? null
    property bool hovered: false
    readonly property var problemCounts: {
        let errors = 0
        let warnings = 0
        for (const problem of clipProblems.problems) {
            if (problem.severity === DocumentProblems.Error)
                ++errors
            else if (problem.severity === DocumentProblems.Warning)
                ++warnings
        }
        return { errors, warnings }
    }

    clip: true

    onClipViewModelChanged: layer.hovered = false

    DocumentProblems {
        id: clipProblems
        windowHandle: layer.ViewLayer.windowHandle
        entity: layer.context?.getClipDocumentItemFromViewItem(layer.clipViewModel) ?? null
    }

    Connections {
        target: layer.clipPaneInteractionController

        function onItemHoverEntered(clipPane, item) {
            if (item === layer.clipViewModel)
                layer.hovered = true
        }
        function onItemHoverExited(clipPane, item) {
            if (item === layer.clipViewModel)
                layer.hovered = false
        }
    }

    Rectangle {
        id: badge

        readonly property bool inViewport: layer.viewport.width > 0 && layer.viewport.height > 0
            && x < layer.viewport.x + layer.viewport.width && x + width > layer.viewport.x
            && y < layer.viewport.y + layer.viewport.height && y + height > layer.viewport.y

        x: Math.min(Math.max(8, layer.viewport.x + 8), Math.max(0, layer.width - width - 8))
        y: 4
        width: counts.implicitWidth + 8
        height: counts.implicitHeight + 8
        visible: inViewport && layer.problemCounts.errors + layer.problemCounts.warnings > 0
        color: Theme.backgroundQuaternaryColor

        ProblemHoverCard {
            anchors.fill: parent
            problems: clipProblems.problems.slice().sort((left, right) => left.severity - right.severity)
            documentProblems: clipProblems
            hovered: badge.visible && layer.hovered
        }

        RowLayout {
            id: counts
            anchors.centerIn: parent
            spacing: 4
            Accessible.role: Accessible.StaticText
            Accessible.name: qsTr("%Ln error(s)", "", layer.problemCounts.errors) + " " + qsTr("%Ln warning(s)", "", layer.problemCounts.warnings)

            ProblemIcon {
                visible: layer.problemCounts.errors > 0
                severity: DocumentProblems.Error
            }
            Label {
                visible: layer.problemCounts.errors > 0
                text: layer.problemCounts.errors.toLocaleString()
                color: Theme.errorColor
            }
            ProblemIcon {
                visible: layer.problemCounts.warnings > 0
                severity: DocumentProblems.Warning
            }
            Label {
                visible: layer.problemCounts.warnings > 0
                text: layer.problemCounts.warnings.toLocaleString()
                color: Theme.warningColor
            }
        }
    }
}
