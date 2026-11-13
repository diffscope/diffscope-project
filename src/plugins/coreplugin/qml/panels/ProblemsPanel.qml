// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: root

    required property DocumentProblems documentProblems

    readonly property Component panelComponent: ActionDockingPane {
        padding: 0
        leftPadding: 2
        rightPadding: leftPadding
        preferredSize: Qt.size(400, 300)
        badgeNumber: root.documentProblems.problems.length
        badgeType: {
            const problems = root.documentProblems.problems
            if (problems.some(problem => problem.severity === DocumentProblems.Error))
                return SVS.CT_Error
            if (problems.some(problem => problem.severity === DocumentProblems.Warning))
                return SVS.CT_Warning
            return SVS.CT_Normal
        }

        ListView {
            id: list

            anchors.fill: parent
            clip: true
            topMargin: 6
            bottomMargin: 6
            spacing: 2
            activeFocusOnTab: true
            keyNavigationEnabled: true
            model: root.documentProblems.problems
            ScrollBar.vertical: ScrollBar {
                interactive: false
            }

            function activate(index) {
                const problem = model[index]
                if (!problem)
                    return
                root.documentProblems.navigate(problem.id)
            }

            Keys.onReturnPressed: activate(currentIndex)
            Keys.onEnterPressed: activate(currentIndex)
            Keys.onMenuPressed: {
                if (currentItem)
                    actionMenu.openFor(currentItem, Qt.point(0, currentItem.height), currentItem.modelData.id)
            }

            ProblemActionMenu {
                id: actionMenu

                documentProblems: root.documentProblems
            }

            delegate: ItemDelegate {
                id: entry

                required property var modelData
                required property int index

                width: list.width
                leftPadding: 6
                rightPadding: leftPadding
                topPadding: 6
                bottomPadding: 6
                down: false
                text: modelData.message

                background: Rectangle {
                    implicitHeight: 28
                    radius: 4
                    color: entry.enabled && entry.hovered
                        ? Theme.controlHoveredColorChange.apply(Theme.buttonColor)
                        : "transparent"
                    border.width: entry.visualFocus ? 2 : 0
                    border.color: Theme.navigationColor
                }

                onClicked: list.activate(index)
                ContextMenu.onRequested: position => actionMenu.openFor(entry, position, modelData.id)

                contentItem: RowLayout {
                    spacing: 8

                    ProblemIcon {
                        Accessible.ignored: true
                        Layout.alignment: Qt.AlignTop
                        severity: entry.modelData.severity
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        horizontalAlignment: entry.mirrored ? Text.AlignRight : Text.AlignLeft
                        text: entry.text
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        color: !entry.enabled ? Theme.foregroundDisabledColorChange.apply(Theme.foregroundPrimaryColor)
                            : entry.hovered ? Theme.foregroundHoveredColorChange.apply(Theme.foregroundPrimaryColor)
                            : Theme.foregroundPrimaryColor
                    }
                }
            }
        }
    }
}
