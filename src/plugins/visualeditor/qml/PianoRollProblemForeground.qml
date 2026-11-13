// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

import SVSCraft.UIComponents

PianoRollProblemLayer {
    id: layer

    property var leftProblem: null
    property var rightProblem: null

    function updateTargets(): void {
        const centerX = viewport.x + viewport.width / 2
        const right = viewport.x + viewport.width
        const bottom = viewport.y + viewport.height
        let nextLeftProblem = null
        let nextRightProblem = null
        let leftX = -Infinity
        let rightX = Infinity
        if (viewport.width > 0 && viewport.height > 0 && timeScale > 0 && keyHeight > 0) {
            for (let i = 0; i < markers.count; ++i) {
                const slot = markers.itemAt(i)
                const problem = slot?.modelData?.problems?.[0]
                if (!problem || problem.severity === DocumentProblems.Notice)
                    continue
                const rect = slot.noteRect
                if (rect.height <= 0)
                    continue
                const outsideLeft = rect.x + rect.width <= viewport.x
                const outsideRight = rect.x >= right
                const outsidePitch = rect.y + rect.height <= viewport.y || rect.y >= bottom
                if (!outsideLeft && !outsideRight && !outsidePitch)
                    continue
                if (outsideLeft || (!outsideRight && rect.x + rect.width / 2 < centerX)) {
                    if (rect.x > leftX) {
                        leftX = rect.x
                        nextLeftProblem = problem
                    }
                } else if (rect.x < rightX) {
                    rightX = rect.x
                    nextRightProblem = problem
                }
            }
        }
        if (leftProblem !== nextLeftProblem)
            leftProblem = nextLeftProblem
        if (rightProblem !== nextRightProblem)
            rightProblem = nextRightProblem
    }

    onViewportChanged: Qt.callLater(layer.updateTargets)

    Item {
        id: markerViewport

        x: layer.viewport.x
        y: layer.viewport.y
        width: layer.viewport.width
        height: layer.viewport.height
        clip: true

        Repeater {
            id: markers

            model: layer.noteProblems
            onItemAdded: Qt.callLater(layer.updateTargets)
            onItemRemoved: Qt.callLater(layer.updateTargets)

            Loader {
                id: markerSlot

                required property var modelData
                readonly property rect noteRect: layer.noteRect(modelData)

                x: noteRect.x + (noteRect.width - width) / 2 - layer.viewport.x
                y: noteRect.y + noteRect.height + 24 - layer.viewport.y
                width: 24
                height: 24
                visible: x + width > 0 && x < markerViewport.width
                    && y + height > 0 && y < markerViewport.height
                active: noteRect.height > 0 && x + width > -width && x < markerViewport.width + width
                    && y + height > -height && y < markerViewport.height + height
                onNoteRectChanged: Qt.callLater(layer.updateTargets)

                sourceComponent: NoteProblemMarker {
                    documentProblems: layer.documentProblems
                    problems: markerSlot.modelData.problems
                }
            }
        }
    }

    component JumpButton: ToolButton {
        id: button

        required property bool jumpsLeft
        readonly property var problem: jumpsLeft ? layer.leftProblem : layer.rightProblem

        visible: problem !== null
        flat: false
        text: qsTr("Jump to Problem")
        DescriptiveText.toolTip: text
        DescriptiveText.activated: hovered || visualFocus

        contentItem: Row {
            spacing: button.spacing
            layoutDirection: button.jumpsLeft ? Qt.RightToLeft : Qt.LeftToRight

            ProblemIcon {
                severity: button.problem?.severity ?? DocumentProblems.Error
            }
            IconImage {
                source: "image://fluent-system-icons/" + (button.jumpsLeft ? "chevron_left" : "chevron_right")
                sourceSize.width: 16
                sourceSize.height: 16
                color: button.icon.color
            }
        }

        onClicked: layer.documentProblems.navigate(problem.id)
    }

    JumpButton {
        jumpsLeft: true
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 8
    }

    JumpButton {
        jumpsLeft: false
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
    }
}
