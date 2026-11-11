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

    property var initialState
    Component.onCompleted: initialState = snapshot()

    function snapshot() {
        const pianoRollPanelInterface = addOn.pianoRollPanelInterface
        const splitView = pianoRollPanelInterface.pianoRollView.noteAreaSplitView
        const bottomPanelHeights = []
        for (let i = 1; i < splitView.count; ++i) {
            const height = splitView.itemAt(i).SplitView.preferredHeight
            bottomPanelHeights.push(height >= 0 ? height : null)
        }
        return {
            tool: pianoRollPanelInterface.tool,
            duration: pianoRollPanelInterface.positionAlignmentManipulator.duration,
            tuplet: pianoRollPanelInterface.positionAlignmentManipulator.tuplet,
            isAutoPageScrollingEnabled: pianoRollPanelInterface.autoPageScrollingManipulator.enabled,
            scaleHighlightEnabled: pianoRollPanelInterface.scaleHighlightEnabled,
            additionalTracks: Array.from(addOn.additionalTrackLoader.loadedComponents),
            bottomAdditionalTracks: Array.from(addOn.bottomAdditionalTrackLoader.loadedComponents),
            bottomPanelHeights: bottomPanelHeights
        }
    }

    readonly property Component pianoRollPanelComponent: ActionDockingPane {
        id: pane
        preferredSize: Qt.size(1152, 480)

        function loadState(state) {
            const initial = d.initialState
            state = state ?? {}
            const durations = [PositionAlignmentManipulator.Auto, PositionAlignmentManipulator.Unset, PositionAlignmentManipulator.Note1st, PositionAlignmentManipulator.Note2nd, PositionAlignmentManipulator.Note4th, PositionAlignmentManipulator.Note8th, PositionAlignmentManipulator.Note16th, PositionAlignmentManipulator.Note32nd, PositionAlignmentManipulator.Note64th, PositionAlignmentManipulator.Note128th]
            const tuplets = [PositionAlignmentManipulator.None, PositionAlignmentManipulator.Triplet, PositionAlignmentManipulator.Quintuplet]
            const validTracks = ids => Array.isArray(ids) && ids.every(id => typeof id === "string")
            const heights = state.bottomPanelHeights
            if (typeof state !== "object" || Array.isArray(state)
                || (state.tool !== undefined && (!Number.isInteger(state.tool) || state.tool < PianoRollPanelInterface.PointerTool || state.tool > PianoRollPanelInterface.PitchAnchorSelectTool))
                || (state.duration !== undefined && !durations.includes(state.duration))
                || (state.tuplet !== undefined && !tuplets.includes(state.tuplet))
                || (state.isAutoPageScrollingEnabled !== undefined && typeof state.isAutoPageScrollingEnabled !== "boolean")
                || (state.scaleHighlightEnabled !== undefined && typeof state.scaleHighlightEnabled !== "boolean")
                || (state.additionalTracks !== undefined && !validTracks(state.additionalTracks))
                || (state.bottomAdditionalTracks !== undefined && !validTracks(state.bottomAdditionalTracks))
                || (heights !== undefined && (!Array.isArray(heights)
                    || heights.length !== (state.bottomAdditionalTracks ?? initial.bottomAdditionalTracks).length
                    || !heights.every(height => height === null || (Number.isFinite(height) && height >= 0)))))
                state = initial

            const loader = d.addOn.additionalTrackLoader
            const bottomLoader = d.addOn.bottomAdditionalTrackLoader
            if (!loader.replaceItems(state.additionalTracks ?? initial.additionalTracks)
                || !bottomLoader.replaceItems(state.bottomAdditionalTracks ?? initial.bottomAdditionalTracks)) {
                state = initial
                loader.replaceItems(initial.additionalTracks)
                bottomLoader.replaceItems(initial.bottomAdditionalTracks)
            }

            const pianoRollPanelInterface = d.addOn.pianoRollPanelInterface
            pianoRollPanelInterface.tool = state.tool ?? initial.tool
            pianoRollPanelInterface.positionAlignmentManipulator.duration = state.duration ?? initial.duration
            pianoRollPanelInterface.positionAlignmentManipulator.tuplet = state.tuplet ?? initial.tuplet
            pianoRollPanelInterface.autoPageScrollingManipulator.enabled = state.isAutoPageScrollingEnabled ?? initial.isAutoPageScrollingEnabled
            pianoRollPanelInterface.scaleHighlightEnabled = state.scaleHighlightEnabled ?? initial.scaleHighlightEnabled

            const splitView = pianoRollPanelInterface.pianoRollView.noteAreaSplitView
            for (let i = 1; i < splitView.count; ++i)
                splitView.itemAt(i).SplitView.preferredHeight = state.bottomPanelHeights?.[i - 1] ?? undefined
        }

        function saveState() {
            return d.snapshot()
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
