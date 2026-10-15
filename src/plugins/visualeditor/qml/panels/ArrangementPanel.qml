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

    readonly property Component arrangementPanelComponent: ActionDockingPane {

        function validateState(state) {
            const durations = [PositionAlignmentManipulator.Auto, PositionAlignmentManipulator.Unset, PositionAlignmentManipulator.Note1st, PositionAlignmentManipulator.Note2nd, PositionAlignmentManipulator.Note4th, PositionAlignmentManipulator.Note8th, PositionAlignmentManipulator.Note16th, PositionAlignmentManipulator.Note32nd, PositionAlignmentManipulator.Note64th, PositionAlignmentManipulator.Note128th]
            const tuplets = [PositionAlignmentManipulator.None, PositionAlignmentManipulator.Triplet, PositionAlignmentManipulator.Quintuplet]
            if (!state || typeof state !== "object" || Array.isArray(state)
                || (state.tool !== undefined && (!Number.isInteger(state.tool) || state.tool < ArrangementPanelInterface.PointerTool || state.tool > ArrangementPanelInterface.HandTool))
                || (state.duration !== undefined && !durations.includes(state.duration))
                || (state.tuplet !== undefined && !tuplets.includes(state.tuplet))
                || (state.isAutoPageScrollingEnabled !== undefined && typeof state.isAutoPageScrollingEnabled !== "boolean")
                || (state.additionalTracks !== undefined && (!Array.isArray(state.additionalTracks) || state.additionalTracks.some((id, index, ids) => typeof id !== "string" || !id.length || ids.indexOf(id) !== index))))
                throw new Error(qsTr("The arrangement panel state is invalid."))
        }

        function prepareState(state) {
            if (!d.addOn.additionalTrackLoader.prepareItems(state.additionalTracks ?? d.addOn.additionalTrackLoader.loadedComponents))
                throw new Error(qsTr("The arrangement panel tracks could not be prepared."))
        }

        function finishState() {
            d.addOn.additionalTrackLoader.finishPreparedItems()
        }

        function abortState() {
            d.addOn.additionalTrackLoader.abortPreparedItems()
        }

        function loadState(state) {
            if (!d.addOn.additionalTrackLoader.commitPreparedItems())
                throw new Error(qsTr("The arrangement panel tracks could not be restored."))
            let arrangementPanelInterface = addOn.arrangementPanelInterface
            if (state.tool !== undefined)
                arrangementPanelInterface.tool = state.tool
            if (state.duration !== undefined)
                arrangementPanelInterface.positionAlignmentManipulator.duration = state.duration
            if (state.tuplet !== undefined)
                arrangementPanelInterface.positionAlignmentManipulator.tuplet = state.tuplet
            if (state.isAutoPageScrollingEnabled !== undefined)
                arrangementPanelInterface.autoPageScrollingManipulator.enabled = state.isAutoPageScrollingEnabled
        }

        function saveState() {
            let arrangementPanelInterface = addOn.arrangementPanelInterface

            return {
                tool: arrangementPanelInterface.tool,
                duration: arrangementPanelInterface.positionAlignmentManipulator.duration,
                tuplet: arrangementPanelInterface.positionAlignmentManipulator.tuplet,
                isAutoPageScrollingEnabled: arrangementPanelInterface.autoPageScrollingManipulator.enabled,
                additionalTracks: d.addOn.additionalTrackLoader.loadedComponents
            }
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
