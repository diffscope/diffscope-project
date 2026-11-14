// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T

import SVSCraft.UIComponents

import DiffScope.Core

T.ToolButton {
    id: marker

    required property var problems
    required property DocumentProblems documentProblems

    implicitWidth: 24
    implicitHeight: 24
    padding: 0
    focusPolicy: Qt.StrongFocus
    hoverEnabled: true
    Accessible.name: card.currentProblem?.message ?? ""

    contentItem: ProblemIcon {
        size: 24
        regular: true
        severity: marker.problems[0]?.severity ?? DocumentProblems.Notice
    }

    background: Rectangle {
        color: "transparent"
        radius: 4
        border.width: marker.visualFocus ? 2 : 0
        border.color: Theme.navigationColor
    }

    function navigateToCurrentProblem(): void {
        if (card.currentProblem)
            documentProblems.navigate(card.currentProblem.id)
    }

    onClicked: navigateToCurrentProblem()
    Keys.onReturnPressed: navigateToCurrentProblem()
    Keys.onEnterPressed: navigateToCurrentProblem()
    Keys.onSpacePressed: card.openDetails(true)
    Accessible.onPressAction: navigateToCurrentProblem()

    ProblemHoverCard {
        id: card

        anchors.fill: parent
        problems: marker.problems
        documentProblems: marker.documentProblems
        hovered: marker.hovered
        mirrored: marker.mirrored
    }
}
