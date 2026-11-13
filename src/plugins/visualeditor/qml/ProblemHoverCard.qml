// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import SVSCraft.UIComponents

Item {
    id: card

    required property var problems
    required property DocumentProblems documentProblems
    required property bool hovered
    property bool mirrored: LayoutMirroring.enabled

    property int currentIndex: 0
    property bool dismissedWhileHovered: false
    readonly property var details: detailsLoader.item
    readonly property var currentProblem: problems[currentIndex] ?? null

    onProblemsChanged: {
        currentIndex = Math.min(currentIndex, Math.max(0, problems.length - 1))
        if (problems.length === 0 && details)
            details.close()
    }
    onVisibleChanged: {
        if (visible)
            return
        if (details)
            details.close()
        else
            detailsLoader.active = false
    }
    onHoveredChanged: {
        if (!hovered)
            dismissedWhileHovered = false
    }

    function openDetails(focus: bool): void {
        if (!visible || !currentProblem)
            return
        dismissedWhileHovered = false
        if (details) {
            details.showProblem(focus)
            return
        }
        detailsLoader.focusOnOpen = focus
        detailsLoader.active = true
    }

    Timer {
        interval: 350
        running: card.visible && card.currentProblem !== null && card.hovered
            && !card.dismissedWhileHovered && !card.details?.visible
        onTriggered: card.openDetails(false)
    }

    Loader {
        id: detailsLoader

        property bool focusOnOpen: false

        active: false
        onLoaded: {
            if (card.visible)
                item.showProblem(focusOnOpen)
        }

        sourceComponent: Popup {
            id: details

            property var problemActions: []
            readonly property Window hostWindow: card.Window.window
            readonly property bool hasFooter: problemActions.length > 0 || card.problems.length > 1

            parent: card
            x: 0
            y: card.height
            width: Math.min(384, Math.max(0, (hostWindow?.width ?? 384) - 16))
            margins: 8
            padding: 0
            closePolicy: Popup.CloseOnPressOutside
            onAboutToShow: updateActions()
            onOpened: {
                if (focus)
                    message.forceActiveFocus()
            }
            onClosed: {
                card.dismissedWhileHovered = card.hovered
                detailsLoader.active = false
            }

            function showProblem(takeFocus: bool): void {
                focus = takeFocus
                if (opened && takeFocus)
                    message.forceActiveFocus()
                else
                    open()
            }

            function updateActions(): void {
                problemActions = card.currentProblem ? card.documentProblems.actions(card.currentProblem.id) : []
            }

            Connections {
                target: card
                function onCurrentProblemChanged() { details.updateActions() }
            }

            Connections {
                target: card.documentProblems
                function onOperationsChanged() { details.updateActions() }
            }

            Shortcut {
                sequence: "Escape"
                enabled: details.visible
                onActivated: details.close()
            }

            Timer {
                interval: 300
                running: details.opened && !card.hovered && !popupHover.hovered
                    && !(details.focus && details.activeFocus)
                onTriggered: details.close()
            }

            contentItem: ColumnLayout {
                spacing: 0

                HoverHandler {
                    id: popupHover
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.margins: 12
                    spacing: 8

                    ProblemIcon {
                        Layout.alignment: Qt.AlignTop
                        severity: card.currentProblem?.severity ?? DocumentProblems.Notice
                    }

                    ScrollView {
                        id: messageView

                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredHeight: Math.min(message.contentHeight, Math.max(24, (details.hostWindow?.height ?? 480) / 3))
                        contentWidth: availableWidth
                        clip: true
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        Flickable {
                            id: messageViewport

                            contentHeight: message.contentHeight

                            TextEdit {
                                id: message

                                width: Math.max(0, messageView.availableWidth - 8)
                                text: card.currentProblem?.message ?? ""
                                textFormat: TextEdit.PlainText
                                wrapMode: TextEdit.Wrap
                                readOnly: true
                                selectByMouse: true
                                activeFocusOnTab: true
                                color: Theme.foregroundPrimaryColor
                                selectionColor: Theme.accentColor
                                font: Theme.font
                                Accessible.role: Accessible.StaticText
                                Accessible.name: text
                                onTextChanged: messageViewport.contentY = 0
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: Theme.borderColor
                    visible: details.hasFooter
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.leftMargin: 8
                    Layout.rightMargin: 8
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                    spacing: 8
                    visible: details.hasFooter

                    Flow {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: 4
                        layoutDirection: card.mirrored ? Qt.RightToLeft : Qt.LeftToRight

                        Repeater {
                            model: details.problemActions
                            ToolButton {
                                required property var modelData
                                width: Math.min(implicitWidth, parent.width)
                                text: modelData.text
                                onClicked: {
                                    if (card.currentProblem)
                                        card.documentProblems.triggerAction(card.currentProblem.id, modelData.id)
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: 2
                        visible: card.problems.length > 1

                        ToolButton {
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/" + (mirrored ? "chevron_right" : "chevron_left")
                            text: qsTr("Previous Problem")
                            onClicked: card.currentIndex = (card.currentIndex + card.problems.length - 1) % card.problems.length
                        }
                        Label {
                            color: Theme.foregroundSecondaryColor
                            text: qsTr("%L1 of %L2").arg(card.currentIndex + 1).arg(card.problems.length)
                        }
                        ToolButton {
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/" + (mirrored ? "chevron_left" : "chevron_right")
                            text: qsTr("Next Problem")
                            onClicked: card.currentIndex = (card.currentIndex + 1) % card.problems.length
                        }
                    }
                }
            }
        }
    }
}
