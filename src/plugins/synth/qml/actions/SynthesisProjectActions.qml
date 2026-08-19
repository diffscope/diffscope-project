// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.DspxModel.SelectionModel as DspxSelectionModel

ActionCollection {
    id: root

    required property QtObject addOn

    ActionItem {
        actionId: "org.diffscope.synth.resynthesize"
        Action {
            enabled: {
                let document = root.addOn?.windowHandle?.projectDocumentContext?.document
                if (!document?.anyItemsSelected)
                    return false
                let selectionModel = document.selectionModel
                let selectionType = selectionModel.selectionType
                if (selectionType === DspxSelectionModel.SelectionModel.ST_Clip)
                    return selectionModel.clipSelectionModel.selectedCount > 0
                           && selectionModel.clipSelectionModel.selectedAudioClipCount === 0
                return selectionType === DspxSelectionModel.SelectionModel.ST_Note
                       && selectionModel.noteSelectionModel.selectedCount > 0
            }
            onTriggered: Qt.callLater(() => root.addOn.resynthesizeSelectedItems())
        }
    }

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
