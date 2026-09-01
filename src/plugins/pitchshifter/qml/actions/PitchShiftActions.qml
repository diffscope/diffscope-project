// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.Core
import DiffScope.DspxModel as DspxModel
import DiffScope.DspxModel.SelectionModel as DspxSelectionModel
import DiffScope.PitchShifter

ActionCollection {
    id: d

    required property PitchShiftAddOn addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null

    ActionItem {
        actionId: "org.diffscope.pitchshifter.edit.shiftPitch"
        Action {
            enabled: d.windowHandle?.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Clip
                     && d.windowHandle?.projectDocumentContext.document.selectionModel.currentItem?.type === DspxModel.Clip.Audio
            onTriggered: Qt.callLater(() => d.addOn.shiftPitch(d.windowHandle.projectDocumentContext.document.selectionModel.currentItem))
        }
    }
}
