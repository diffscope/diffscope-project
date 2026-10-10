// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick.Controls

import QActionKit

import DiffScope.Core
import DiffScope.DspxModel as DspxModel
import DiffScope.DspxModel.SelectionModel as DspxSelectionModel
import DiffScope.Synth

ActionCollection {
    id: d

    required property AudioExtractionAddOn addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property var selection: windowHandle?.projectDocumentContext.document.selectionModel
    readonly property bool audioClipSelected: selection?.selectionType === DspxSelectionModel.SelectionModel.ST_Clip
                                               && selection?.currentItem?.kind === DspxModel.Clip.Audio

    ActionItem {
        actionId: "org.diffscope.synth.extract.notes"
        Action {
            enabled: d.audioClipSelected
            onTriggered: Qt.callLater(() => d.addOn.extractNotes(d.selection.currentItem))
        }
    }
    ActionItem {
        actionId: "org.diffscope.synth.extract.separation"
        Action {
            enabled: d.audioClipSelected
            onTriggered: Qt.callLater(() => d.addOn.separateAudio(d.selection.currentItem))
        }
    }
}
