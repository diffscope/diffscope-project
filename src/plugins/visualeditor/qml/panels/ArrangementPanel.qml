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

import DiffScope.UIShell
import DiffScope.VisualEditor

QtObject {
    id: d
    required property QtObject addOn
    required property QtObject scrollAddOn

    property var initialState
    Component.onCompleted: initialState = snapshot()

    function snapshot() {
        const arrangementPanelInterface = addOn.arrangementPanelInterface
        return {
            tool: arrangementPanelInterface.tool,
            duration: arrangementPanelInterface.positionAlignmentManipulator.duration,
            tuplet: arrangementPanelInterface.positionAlignmentManipulator.tuplet,
            isAutoPageScrollingEnabled: arrangementPanelInterface.autoPageScrollingManipulator.enabled,
            additionalTracks: Array.from(addOn.additionalTrackLoader.loadedComponents)
        }
    }

    readonly property Component arrangementPanelComponent: ActionDockingPane {
        preferredSize: Qt.size(1152, 320)

        function loadState(state) {
            const initial = d.initialState
            state = state ?? {}
            const durations = [PositionAlignmentManipulator.Auto, PositionAlignmentManipulator.Unset, PositionAlignmentManipulator.Note1st, PositionAlignmentManipulator.Note2nd, PositionAlignmentManipulator.Note4th, PositionAlignmentManipulator.Note8th, PositionAlignmentManipulator.Note16th, PositionAlignmentManipulator.Note32nd, PositionAlignmentManipulator.Note64th, PositionAlignmentManipulator.Note128th]
            const tuplets = [PositionAlignmentManipulator.None, PositionAlignmentManipulator.Triplet, PositionAlignmentManipulator.Quintuplet]
            if (typeof state !== "object" || Array.isArray(state)
                || (state.tool !== undefined && (!Number.isInteger(state.tool) || state.tool < ArrangementPanelInterface.PointerTool || state.tool > ArrangementPanelInterface.HandTool))
                || (state.duration !== undefined && !durations.includes(state.duration))
                || (state.tuplet !== undefined && !tuplets.includes(state.tuplet))
                || (state.isAutoPageScrollingEnabled !== undefined && typeof state.isAutoPageScrollingEnabled !== "boolean")
                || (state.additionalTracks !== undefined && (!Array.isArray(state.additionalTracks) || !state.additionalTracks.every(id => typeof id === "string"))))
                state = initial

            const loader = d.addOn.additionalTrackLoader
            if (!loader.replaceItems(state.additionalTracks ?? initial.additionalTracks)) {
                state = initial
                loader.replaceItems(initial.additionalTracks)
            }

            const arrangementPanelInterface = d.addOn.arrangementPanelInterface
            arrangementPanelInterface.tool = state.tool ?? initial.tool
            arrangementPanelInterface.positionAlignmentManipulator.duration = state.duration ?? initial.duration
            arrangementPanelInterface.positionAlignmentManipulator.tuplet = state.tuplet ?? initial.tuplet
            arrangementPanelInterface.autoPageScrollingManipulator.enabled = state.isAutoPageScrollingEnabled ?? initial.isAutoPageScrollingEnabled
        }

        function saveState() {
            return d.snapshot()
        }

        Docking.onActivated: d.scrollAddOn.activeEditingArea = 0

        Component {
            id: dummyItem

            Item {}
        }
        header: ToolBarContainer {
            id: toolBar
            anchors.fill: parent

            property MenuActionInstantiator instantiator: MenuActionInstantiator {
                actionId: "org.diffscope.visualeditor.arrangementPanelToolBar"
                context: d.addOn?.windowHandle.actionContext ?? null
                separatorComponent: ToolBarContainerSeparator {}
                stretchComponent: ToolBarContainerStretch {}
                Component.onCompleted: forceUpdateLayouts()
            }
        }
        data: [d.addOn?.arrangementPanelInterface.arrangementView ?? null]
    }
}
