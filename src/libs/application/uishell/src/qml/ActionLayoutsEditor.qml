import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import QtQuick.Templates as T
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

TreeView {
    id: treeView

    delegate: T.TreeViewDelegate {
        id: control

        implicitWidth: leftMargin + __contentIndent + implicitContentWidth + rightPadding + rightMargin
        implicitHeight: 24

        font: Theme.font

        indentation: indicator ? indicator.width : 12
        leftMargin: 4
        rightMargin: 4
        spacing: 4

        topPadding: contentItem ? (height - contentItem.implicitHeight) / 2 : 0
        leftPadding: !mirrored ? leftMargin + __contentIndent : width - leftMargin - __contentIndent - implicitContentWidth

        highlighted: control.selected || control.current
                    || ((control.treeView.selectionBehavior === TableView.SelectRows
                    || control.treeView.selectionBehavior === TableView.SelectionDisabled)
                    && control.row === control.treeView.currentRow)

        text: model.display

        width: treeView.width
        onWidthChanged: width = treeView.width

        Accessible.role: Accessible.TreeItem

        required property int row
        required property var model
        readonly property real __contentIndent: !isTreeNode ? 0 : (depth * indentation) + (indicator ? indicator.width + spacing : 0)

        indicator: Item {
            readonly property real __indicatorIndent: control.leftMargin + (control.depth * control.indentation)
            x: !control.mirrored ? __indicatorIndent : control.width - __indicatorIndent - width
            y: (control.height - height) / 2
            implicitWidth: 16
            implicitHeight: 24
            ColorImage {
                id: arrow
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                width: 12
                height: 12
                source: "qrc:/qt/qml/DiffScope/UIShell/assets/ChevronRight12Filled.svg"
                color: !control.enabled ? Theme.foregroundDisabledColorChange.apply(Theme.foregroundPrimaryColor) :
                    control.down ? Theme.foregroundPressedColorChange.apply(Theme.foregroundPrimaryColor) :
                        control.hovered ? Theme.foregroundHoveredColorChange.apply(Theme.foregroundPrimaryColor) :
                            Theme.foregroundPrimaryColor
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
            property color _baseColor: control.highlighted ? Theme.accentColor : Theme.buttonColor
            property color _statusColor: !control.enabled ? Theme.controlDisabledColorChange.apply(_baseColor) :
                control.down ? Theme.controlPressedColorChange.apply(_baseColor) :
                    control.hovered ? Theme.controlHoveredColorChange.apply(_baseColor) :
                        transparentIf(!control.highlighted, _baseColor)
            color: _statusColor

        }

        contentItem: Text {
            clip: false
            text: control.text
            font: control.font
            elide: Text.ElideRight
            color: !control.enabled ? Theme.foregroundDisabledColorChange.apply(Theme.foregroundPrimaryColor) :
                   control.down ? Theme.foregroundPressedColorChange.apply(Theme.foregroundPrimaryColor) :
                   control.hovered ? Theme.foregroundHoveredColorChange.apply(Theme.foregroundPrimaryColor) :
                   control.highlighted ? Theme.foregroundPrimaryColor : Theme.foregroundPrimaryColor
            visible: !control.editing
        }
    }
}