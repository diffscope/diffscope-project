import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Layouts

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn
    required property ProjectViewModelContext projectViewModelContext

    readonly property Component tempoTrackComponent: LabelSequence {
        required property QtObject contextObject
        Layout.fillWidth: true
        labelSequenceViewModel: d.projectViewModelContext?.tempoSequenceViewModel ?? null
        scrollBehaviorViewModel: contextObject?.scrollBehaviorViewModel ?? null
        timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
        timeViewModel: contextObject?.timeViewModel ?? null
        labelSequenceInteractionController: contextObject?.labelSequenceInteractionControllerOfTempo ?? null
        selectionController: d.projectViewModelContext?.tempoSelectionController ?? null
    }

}