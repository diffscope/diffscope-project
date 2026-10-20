// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import "JsonState.js" as JsonState

ScrollView {
    id: control

    contentWidth: availableWidth

    signal stateEdited()

    QtObject {
        id: d

        property var currentState: ({})
    }

    function validateState(state) {
        if (!state || typeof state !== "object" || Array.isArray(state))
            return false
        if (state.steps !== undefined) {
            if (!state.steps || typeof state.steps !== "object" || Array.isArray(state.steps))
                return false
            for (const stage of ["pitch", "variance", "acoustic"]) {
                const value = state.steps[stage]
                if (value !== undefined && (!Number.isSafeInteger(value) || value <= 0))
                    return false
            }
        }
        return state.depth === undefined || (typeof state.depth === "number"
                && Number.isFinite(state.depth) && state.depth >= 0 && state.depth <= 1)
    }

    function loadState(state) {
        if (!validateState(state))
            return false
        d.currentState = JsonState.copy(state)
    }

    function saveState() {
        return JsonState.copy(d.currentState)
    }

    function setSteps(stage, value) {
        if ((d.currentState.steps?.[stage] ?? 20) === value)
            return
        const state = JsonState.copy(d.currentState)
        if (state.steps === undefined)
            state.steps = {}
        state.steps[stage] = value
        d.currentState = state
        stateEdited()
    }

    function setDepth(value) {
        if ((d.currentState.depth ?? 1) === value)
            return
        const state = JsonState.copy(d.currentState)
        state.depth = value
        d.currentState = state
        stateEdited()
    }

    ColumnLayout {
        width: control.availableWidth

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Synthesis Options")

            GridLayout {
                anchors.fill: parent
                columns: 2
                columnSpacing: 12
                rowSpacing: 8

                Label {
                    id: pitchStepsLabel
                    text: qsTr("Pitch steps")
                }
                DoubleSpinBox {
                    decimals: 0
                    from: 1
                    to: Number.MAX_SAFE_INTEGER
                    stepSize: 1
                    editable: true
                    value: d.currentState.steps?.pitch ?? 20
                    Accessible.labelledBy: pitchStepsLabel
                    onValueModified: control.setSteps("pitch", value)
                }

                Label {
                    id: varianceStepsLabel
                    text: qsTr("Variance steps")
                }
                DoubleSpinBox {
                    decimals: 0
                    from: 1
                    to: Number.MAX_SAFE_INTEGER
                    stepSize: 1
                    editable: true
                    value: d.currentState.steps?.variance ?? 20
                    Accessible.labelledBy: varianceStepsLabel
                    onValueModified: control.setSteps("variance", value)
                }

                Label {
                    id: acousticStepsLabel
                    text: qsTr("Acoustic steps")
                }
                DoubleSpinBox {
                    decimals: 0
                    from: 1
                    to: Number.MAX_SAFE_INTEGER
                    stepSize: 1
                    editable: true
                    value: d.currentState.steps?.acoustic ?? 20
                    Accessible.labelledBy: acousticStepsLabel
                    onValueModified: control.setSteps("acoustic", value)
                }

                Label {
                    id: depthLabel
                    text: qsTr("Depth")
                }
                DoubleSpinBox {
                    decimals: 3
                    from: 0
                    to: 1
                    stepSize: 0.01
                    editable: true
                    value: d.currentState.depth ?? 1
                    Accessible.labelledBy: depthLabel
                    onValueModified: control.setDepth(value)
                }
            }
        }
    }
}
