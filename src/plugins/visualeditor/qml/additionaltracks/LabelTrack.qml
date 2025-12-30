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

    readonly property ProjectViewModelContext projectViewModelContext: addOn?.windowHandle.ProjectViewModelContext.context ?? null

    readonly property Component labelTrackComponent: LabelSequence {
        id: control

        required property QtObject contextObject
        Layout.fillWidth: true
        labelSequenceViewModel: d.projectViewModelContext?.labelSequenceViewModel ?? null
        scrollBehaviorViewModel: contextObject?.scrollBehaviorViewModel ?? null
        timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
        timeViewModel: contextObject?.timeViewModel ?? null
        labelSequenceInteractionController: contextObject?.labelSequenceInteractionControllerOfLabel ?? null
        selectionController: d.projectViewModelContext?.labelSelectionController ?? null

        property LabelViewModel itemBeingDragged: null

        // TODO move this to C++ code
        Connections {
            target: control.itemBeingDragged
            function onPositionChanged() {
                control.timeLayoutViewModel.cursorPosition = control.itemBeingDragged.position
            }
        }

        Connections {
            target: control.labelSequenceInteractionController
            function onMovingStarted(labelSequence, item) {
                if (labelSequence === control) {
                    control.itemBeingDragged = item
                }
            }
            function onMovingFinished(labelSequence, item) {
                if (labelSequence === control) {
                    control.itemBeingDragged = null
                }
            }

            function onRubberBandDraggingStarted() {
                control.timeLayoutViewModel.cursorPosition = -1
            }

        }
    }

}