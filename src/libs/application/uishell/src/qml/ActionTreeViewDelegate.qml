// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls.impl as Impl
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents

T.TreeViewDelegate {
    id: control

    required property ActionTreeView actionTreeView
    required property int row
    required property var model
    readonly property var modelIndex: actionTreeView.index(row, 0)
    readonly property real contentIndent: !isTreeNode ? 0 : depth * indentation + (indicator ? indicator.width + spacing : 0)
    readonly property color primaryTextColor: !enabled ? Theme.foregroundDisabledColorChange.apply(Theme.foregroundPrimaryColor) :
                                             down ? Theme.foregroundPressedColorChange.apply(Theme.foregroundPrimaryColor) :
                                             hovered ? Theme.foregroundHoveredColorChange.apply(Theme.foregroundPrimaryColor) :
                                             Theme.foregroundPrimaryColor
    readonly property color secondaryTextColor: !enabled ? Theme.foregroundDisabledColorChange.apply(Theme.foregroundSecondaryColor) :
                                               down ? Theme.foregroundPressedColorChange.apply(Theme.foregroundSecondaryColor) :
                                               hovered ? Theme.foregroundHoveredColorChange.apply(Theme.foregroundSecondaryColor) :
                                               Theme.foregroundSecondaryColor

    implicitWidth: leftPadding + implicitContentWidth + rightPadding
    implicitHeight: 24
    font: Theme.font
    focusPolicy: Qt.StrongFocus
    indentation: indicator ? indicator.width : 12
    leftMargin: 4
    rightMargin: 4
    spacing: 4
    topPadding: contentItem ? (height - contentItem.implicitHeight) / 2 : 0
    leftPadding: mirrored ? rightMargin : leftMargin + contentIndent
    rightPadding: mirrored ? leftMargin + contentIndent : rightMargin
    highlighted: current || selected
    icon.color: primaryTextColor
    icon.width: 16
    icon.height: 16
    Accessible.name: text
    Accessible.role: Accessible.TreeItem

    onClicked: {
        actionTreeView.forceActiveFocus()
        actionTreeView.selectIndex(modelIndex)
    }
    onDoubleClicked: actionTreeView.activateIndex(modelIndex)

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: eventPoint => {
            control.actionTreeView.forceActiveFocus()
            control.actionTreeView.selectIndex(control.modelIndex)
            const point = control.mapToItem(control.actionTreeView, eventPoint.position.x, eventPoint.position.y)
            control.actionTreeView.contextMenuRequested(point.x, point.y)
        }
    }

    indicator: Item {
        readonly property real indicatorIndent: control.leftMargin + control.depth * control.indentation
        x: !control.mirrored ? indicatorIndent : control.width - indicatorIndent - width
        y: (control.height - height) / 2
        implicitWidth: 16
        implicitHeight: 24
        Impl.ColorImage {
            id: arrow
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: 12
            height: 12
            sourceSize.width: 12
            sourceSize.height: 12
            source: "image://fluent-system-icons/chevron_right?size=12"
            color: control.primaryTextColor
            transform: [
                Rotation {
                    origin.x: arrow.width / 2
                    origin.y: arrow.height / 2
                    angle: control.expanded ? 90 : 0
                },
                Scale {
                    origin.x: arrow.width / 2
                    origin.y: arrow.height / 2
                    xScale: control.mirrored ? -1 : 1
                }
            ]
            Behavior on color {
                ColorAnimation {
                    duration: Theme.colorAnimationDuration
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    background: Rectangle {
        implicitHeight: 24
        function transparentIf(condition, color) {
            return condition ? Qt.rgba(color.r, color.g, color.b, 0) : color
        }
        readonly property color baseColor: control.highlighted ? Theme.accentColor : Theme.buttonColor
        color: !control.enabled ? Theme.controlDisabledColorChange.apply(baseColor) :
               control.down ? Theme.controlPressedColorChange.apply(baseColor) :
               control.hovered ? Theme.controlHoveredColorChange.apply(baseColor) :
               transparentIf(!control.highlighted, baseColor)
    }
}
