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

    property QtObject actionRegistry: null
    property string filterText
    clip: true
    acceptedButtons: Qt.NoButton
    boundsBehavior: Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    reuseItems: false

    selectionModel: ItemSelectionModel {
    }

    ActionLayoutsEditorHelper {
        id: helper
        actionRegistry: treeView.actionRegistry
    }

    delegate: T.TreeViewDelegate {
        id: control

        readonly property var info: helper.getActionDisplayInfo(model.display, model.entry)

        implicitWidth: leftMargin + __contentIndent + implicitContentWidth + rightPadding + rightMargin
        implicitHeight: 24

        font: Theme.font

        indentation: indicator ? indicator.width : 12
        leftMargin: 4
        rightMargin: 4
        spacing: 4

        topPadding: contentItem ? (height - contentItem.implicitHeight) / 2 : 0
        leftPadding: !mirrored ? leftMargin + __contentIndent : width - leftMargin - __contentIndent - implicitContentWidth

        highlighted: control.current || control.selected
        text: control.info.text
        icon.source: info.iconSource
        icon.color: info.iconColor.valid ? info.iconColor : Theme.foregroundPrimaryColor
        icon.width: 16
        icon.height: 16

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
                sourceSize.width: 12
                sourceSize.height: 12
                source: "image://fluent-system-icons/chevron_right?size=12"
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

        contentItem: RowLayout {
            spacing: control.spacing
            IconLabel {
                id: iconLabel
                icon: control.icon
                color: label.color
                visible: width !== 0 && control.info.type !== ActionLayoutsEditorHelper.Separator && control.info.type !== ActionLayoutsEditorHelper.Stretch
            }
            IconLabel {
                id: separatorIcon
                visible: control.info.type === ActionLayoutsEditorHelper.Separator || control.info.type === ActionLayoutsEditorHelper.Stretch
                icon.source: control.info.type === ActionLayoutsEditorHelper.Separator ? "image://fluent-system-icons/line_horizontal_1_dashes" : "image://fluent-system-icons/auto_fit_width"
                icon.color: typeLabel.color
                icon.width: 16
                icon.height: 16
            }
            Text {
                id: label
                text: helper.highlightString(control.text, treeView.filterText, Theme.highlightColor)
                font: control.font
                color: !control.enabled ? Theme.foregroundDisabledColorChange.apply(Theme.foregroundPrimaryColor) :
                       control.down ? Theme.foregroundPressedColorChange.apply(Theme.foregroundPrimaryColor) :
                       control.hovered ? Theme.foregroundHoveredColorChange.apply(Theme.foregroundPrimaryColor) :
                       control.highlighted ? Theme.foregroundPrimaryColor : Theme.foregroundPrimaryColor
                textFormat: Text.RichText
                visible: control.info.type !== ActionLayoutsEditorHelper.Separator && control.info.type !== ActionLayoutsEditorHelper.Stretch
            }
            Text {
                id: typeLabel
                text: {
                    if (control.info.topLevel) {
                        return ""
                    }
                    if (control.info.type === ActionLayoutsEditorHelper.Action) {
                        return qsTr("Action")
                    }
                    if (control.info.type === ActionLayoutsEditorHelper.Menu) {
                        return qsTr("Menu")
                    }
                    if (control.info.type === ActionLayoutsEditorHelper.Group) {
                        return qsTr("Action group")
                    }
                    if (control.info.type === ActionLayoutsEditorHelper.Separator) {
                        return qsTr("Separator")
                    }
                    if (control.info.type === ActionLayoutsEditorHelper.Stretch) {
                        return qsTr("Stretch")
                    }
                }
                font: control.font
                color: !control.enabled ? Theme.foregroundDisabledColorChange.apply(Theme.foregroundSecondaryColor) :
                       control.down ? Theme.foregroundPressedColorChange.apply(Theme.foregroundSecondaryColor) :
                       control.hovered ? Theme.foregroundHoveredColorChange.apply(Theme.foregroundSecondaryColor) :
                       control.highlighted ? Theme.foregroundSecondaryColor : Theme.foregroundSecondaryColor
                Layout.fillWidth: true
                leftPadding: control.info.type === ActionLayoutsEditorHelper.Separator || control.info.type === ActionLayoutsEditorHelper.Stretch ? 0 : 8
            }
        }
    }
}