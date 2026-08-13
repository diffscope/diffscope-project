pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

ActionCollection {
    id: root

    required property QtObject addOn

    ActionItem {
        actionId: "org.diffscope.synth.status"

        RowLayout {
            id: indicator
            spacing: 4
            readonly property int count: root.addOn.synthesisContext.synthesizingPieceCount + root.addOn.synthesisContext.queuedPieceCount
            Accessible.role: Accessible.StaticText
            Layout.fillWidth: false
            Accessible.name: count > 0 ? qsTr("%Ln synthesis task(s)", "", count) : qsTr("No synthesis tasks")
            DescriptiveText.toolTip: Accessible.name
            DescriptiveText.activated: hoverHandler.hovered
            IconLabel {
                icon.source: "image://fluent-system-icons/music_note_2_pulse"
                icon.width: 14
                icon.height: 14
                leftPadding: 4
                rightPadding: 4
                text: indicator.count > 0 ? indicator.count.toLocaleString() : ""
                icon.color: indicator.count > 0 ? Theme.foregroundPrimaryColor : Theme.foregroundSecondaryColor
                color: indicator.count > 0 ? Theme.foregroundPrimaryColor : Theme.foregroundSecondaryColor
                spacing: 2
            }
            HoverHandler {
                id: hoverHandler
            }
        }
    }
}
