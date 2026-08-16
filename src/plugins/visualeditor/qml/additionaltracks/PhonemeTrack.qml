// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Layouts

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views as ScopicFlowViews

import DiffScope.Core
import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn

    readonly property ProjectViewModelContext projectViewModelContext: addOn?.windowHandle.ProjectViewModelContext.context ?? null

    readonly property Component phonemeTrackComponent: ScopicFlowViews.PhonemeSequence {
        id: control
        required property PianoRollPanelInterface contextObject

        property PhonemeViewModel itemBeingDragged: null
        property bool phonemeHovered: false
        property int hoverCursorPosition: -1
        property bool timeCursorOverrideActive: false
        readonly property bool timeCursorOverrideRequested:
            itemBeingDragged !== null
            || (phonemeHovered
                && !(contextObject?.mouseTrackingDisabled ?? true))

        implicitHeight: 40
        Layout.fillWidth: true
        Layout.fillHeight: true

        function itemTimelinePosition(item): int {
            return control.sequencePositionToTimeline(
                item?.sequencePosition ?? 0)
        }

        function sequencePositionToTimeline(position): int {
            return Math.round(
                position
                + (control.clipViewModel?.position ?? 0)
                - (control.clipViewModel?.clipStart ?? 0))
        }

        function updatePhonemeHover(phonemeSequence, position) {
            if (phonemeSequence !== control
                    || (control.contextObject?.mouseTrackingDisabled ?? true)) {
                return
            }
            control.hoverCursorPosition =
                control.sequencePositionToTimeline(position)
            control.phonemeHovered = true
        }

        function setTimeCursorOverrideActive(active: bool) {
            if (control.timeCursorOverrideActive === active)
                return
            control.timeCursorOverrideActive = active
            if (active) {
                control.contextObject?.pianoRollView?.beginTimeCursorOverride(control)
            } else {
                control.contextObject?.pianoRollView?.endTimeCursorOverride(control)
            }
        }

        onTimeCursorOverrideRequestedChanged:
            setTimeCursorOverrideActive(timeCursorOverrideRequested)

        phonemeSequenceViewModel:
            d.projectViewModelContext?.getPhonemeSequenceViewModel(
                contextObject?.editingClip ?? null) ?? null
        noteSequenceViewModel:
            d.projectViewModelContext?.getNoteSequenceViewModel(
                contextObject?.editingClip ?? null) ?? null
        clipViewModel:
            d.projectViewModelContext?.getClipViewItemFromDocumentItem(
                contextObject?.editingClip ?? null) ?? null
        timeViewModel: contextObject?.timeViewModel ?? null
        timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
        phonemeSequenceInteractionController:
            contextObject?.phonemeSequenceInteractionController ?? null

        Item {
            id: backgroundContainer

            anchors.fill: parent
            z: -1

            property Item item: null
            data: [item]

            Component.onCompleted: Qt.callLater(() => {
                const component = d.addOn?.phonemePanelBackgroundComponent
                if (!component)
                    return
                item = component.createObject(backgroundContainer, {
                    clipViewModel: control.clipViewModel,
                    projectViewModelContext: d.projectViewModelContext,
                    timeViewModel: control.timeViewModel,
                    timeLayoutViewModel: control.timeLayoutViewModel
                })
                if (!item)
                    return
                item.width = Qt.binding(() => backgroundContainer.width)
                item.height = Qt.binding(() => backgroundContainer.height)
                item.clipViewModel = Qt.binding(() => control.clipViewModel)
                item.projectViewModelContext = Qt.binding(
                    () => d.projectViewModelContext)
                item.timeViewModel = Qt.binding(() => control.timeViewModel)
                item.timeLayoutViewModel = Qt.binding(
                    () => control.timeLayoutViewModel)
            })
        }

        Binding {
            target: control.contextObject?.timeLayoutViewModel ?? null
            property: "cursorPosition"
            value: control.itemBeingDragged !== null
                ? control.itemTimelinePosition(control.itemBeingDragged)
                : control.hoverCursorPosition
            when: control.timeCursorOverrideRequested
        }

        Connections {
            target: control.phonemeSequenceInteractionController

            function onHoverEntered(phonemeSequence, position) {
                control.updatePhonemeHover(phonemeSequence, position)
            }

            function onHoverMoved(phonemeSequence, position) {
                control.updatePhonemeHover(phonemeSequence, position)
            }

            function onHoverExited(phonemeSequence) {
                if (phonemeSequence === control)
                    control.phonemeHovered = false
            }

            function onMovingStarted(phonemeSequence, item) {
                if (phonemeSequence === control)
                    control.itemBeingDragged = item
            }

            function onMovingCommitted(phonemeSequence) {
                if (phonemeSequence === control)
                    control.itemBeingDragged = null
            }

            function onMovingAborted(phonemeSequence) {
                if (phonemeSequence === control)
                    control.itemBeingDragged = null
            }
        }

        Connections {
            target: control.contextObject

            function onMouseTrackingDisabledChanged() {
                if (control.contextObject?.mouseTrackingDisabled ?? true)
                    control.phonemeHovered = false
            }
        }

        Component.onDestruction:
            setTimeCursorOverrideActive(false)
    }
}
