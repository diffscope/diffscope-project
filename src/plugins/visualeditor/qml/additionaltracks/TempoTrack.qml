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
        id: control

        required property QtObject contextObject
        Layout.fillWidth: true
        labelSequenceViewModel: d.projectViewModelContext?.tempoSequenceViewModel ?? null
        scrollBehaviorViewModel: contextObject?.scrollBehaviorViewModel ?? null
        timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
        timeViewModel: contextObject?.timeViewModel ?? null
        labelSequenceInteractionController: contextObject?.labelSequenceInteractionControllerOfTempo ?? null
        selectionController: d.projectViewModelContext?.tempoSelectionController ?? null

        property LabelViewModel itemBeingDragged: null

        Connections {
            target: control.itemBeingDragged
            function onPositionChanged() {
                control.timeLayoutViewModel.cursorPosition = control.itemBeingDragged.position
            }
        }

        Connections {
            target: control.labelSequenceInteractionController
            function onItemInteractionOperationStarted(labelSequence, item, interactionFlag) {
                if (labelSequence === control && interactionFlag === LabelSequenceInteractionController.Move) {
                    control.itemBeingDragged = item
                }
            }
            function onItemInteractionOperationFinished(labelSequence, item, interactionFlag) {
                if (labelSequence === control && interactionFlag === LabelSequenceInteractionController.Move) {
                    control.itemBeingDragged = null
                }
            }
        }
    }

}