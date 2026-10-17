// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.VisualEditor

QtObject {
    id: d
    required property QtObject addOn
    required property QtObject scrollAddOn

    readonly property Component pianoRollPanelComponent: ActionDockingPane {
        id: pane
        preferredSize: Qt.size(1152, 480)

        function validateState(state) {
            const durations = [PositionAlignmentManipulator.Auto, PositionAlignmentManipulator.Unset, PositionAlignmentManipulator.Note1st, PositionAlignmentManipulator.Note2nd, PositionAlignmentManipulator.Note4th, PositionAlignmentManipulator.Note8th, PositionAlignmentManipulator.Note16th, PositionAlignmentManipulator.Note32nd, PositionAlignmentManipulator.Note64th, PositionAlignmentManipulator.Note128th]
            const tuplets = [PositionAlignmentManipulator.None, PositionAlignmentManipulator.Triplet, PositionAlignmentManipulator.Quintuplet]
            const validTracks = ids => Array.isArray(ids) && !ids.some((id, index) => typeof id !== "string" || !id.length || ids.indexOf(id) !== index)
            if (!state || typeof state !== "object" || Array.isArray(state)
                || (state.tool !== undefined && (!Number.isInteger(state.tool) || state.tool < PianoRollPanelInterface.PointerTool || state.tool > PianoRollPanelInterface.PitchAnchorSelectTool))
                || (state.duration !== undefined && !durations.includes(state.duration))
                || (state.tuplet !== undefined && !tuplets.includes(state.tuplet))
                || (state.isAutoPageScrollingEnabled !== undefined && typeof state.isAutoPageScrollingEnabled !== "boolean")
                || (state.scaleHighlightEnabled !== undefined && typeof state.scaleHighlightEnabled !== "boolean")
                || (state.additionalTracks !== undefined && !validTracks(state.additionalTracks))
                || (state.bottomAdditionalTracks !== undefined && !validTracks(state.bottomAdditionalTracks)))
                throw new Error(qsTr("The piano roll panel state is invalid."))
        }

        function prepareState(state) {
            if (!d.addOn.additionalTrackLoader.prepareItems(state.additionalTracks ?? d.addOn.additionalTrackLoader.loadedComponents)
                || !d.addOn.bottomAdditionalTrackLoader.prepareItems(state.bottomAdditionalTracks ?? d.addOn.bottomAdditionalTrackLoader.loadedComponents))
                throw new Error(qsTr("The piano roll panel tracks could not be prepared."))
        }

        function finishState() {
            d.addOn.additionalTrackLoader.finishPreparedItems()
            d.addOn.bottomAdditionalTrackLoader.finishPreparedItems()
        }

        function abortState() {
            d.addOn.additionalTrackLoader.abortPreparedItems()
            d.addOn.bottomAdditionalTrackLoader.abortPreparedItems()
        }

        function loadState(state) {
            if (!d.addOn.additionalTrackLoader.commitPreparedItems()
                || !d.addOn.bottomAdditionalTrackLoader.commitPreparedItems())
                throw new Error(qsTr("The piano roll panel tracks could not be restored."))
            let pianoRollPanelInterface = addOn.pianoRollPanelInterface
            if (state.tool !== undefined)
                pianoRollPanelInterface.tool = state.tool
            if (state.duration !== undefined)
                pianoRollPanelInterface.positionAlignmentManipulator.duration = state.duration
            if (state.tuplet !== undefined)
                pianoRollPanelInterface.positionAlignmentManipulator.tuplet = state.tuplet
            if (state.isAutoPageScrollingEnabled !== undefined)
                pianoRollPanelInterface.autoPageScrollingManipulator.enabled = state.isAutoPageScrollingEnabled
            if (state.scaleHighlightEnabled !== undefined)
                pianoRollPanelInterface.scaleHighlightEnabled = state.scaleHighlightEnabled
            if (state.noteAreaSplitViewState !== undefined)
                if (!pianoRollPanelInterface.pianoRollView.noteAreaSplitView.restoreState(state.noteAreaSplitViewState))
                    throw new Error(qsTr("The piano roll split view state is invalid."))
        }

        function saveState() {
            let pianoRollPanelInterface = addOn.pianoRollPanelInterface

            return {
                tool: pianoRollPanelInterface.tool,
                duration: pianoRollPanelInterface.positionAlignmentManipulator.duration,
                tuplet: pianoRollPanelInterface.positionAlignmentManipulator.tuplet,
                isAutoPageScrollingEnabled: pianoRollPanelInterface.autoPageScrollingManipulator.enabled,
                scaleHighlightEnabled: pianoRollPanelInterface.scaleHighlightEnabled,
                additionalTracks: d.addOn.additionalTrackLoader.loadedComponents,
                bottomAdditionalTracks: d.addOn.bottomAdditionalTrackLoader.loadedComponents,
                noteAreaSplitViewState: pianoRollPanelInterface.pianoRollView.noteAreaSplitView.saveState()
            }
        }

        Component {
            id: dummyItem

            Item {}
        }

        Docking.onActivated: d.scrollAddOn.activeEditingArea = 1

        header: ToolBarContainer {
            anchors.fill: parent

            property MenuActionInstantiator instantiator: MenuActionInstantiator {
                actionId: "org.diffscope.visualeditor.pianoRollPanelToolBar"
                context: d.addOn?.windowHandle.actionContext ?? null
                separatorComponent: ToolBarContainerSeparator {}
                stretchComponent: ToolBarContainerStretch {}
                Component.onCompleted: forceUpdateLayouts()
            }
        }
        data: [d.addOn?.pianoRollPanelInterface.pianoRollView ?? null]
    }
}
