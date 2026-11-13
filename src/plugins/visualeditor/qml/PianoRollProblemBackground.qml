// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

import SVSCraft.UIComponents

PianoRollProblemLayer {
    id: layer

    z: 1

    Item {
        x: layer.viewport.x
        y: layer.viewport.y
        width: layer.viewport.width
        height: layer.viewport.height
        clip: true

        Repeater {
            model: layer.noteProblems.filter(note => note.problems[0].severity !== DocumentProblems.Notice)

            Rectangle {
                required property var modelData
                x: (layer.noteStartTick(modelData) - layer.timeStart) * layer.timeScale + 0.5 - layer.viewport.x
                width: Math.max(0, (modelData.noteModel?.length ?? 0) * layer.timeScale - 1)
                height: layer.viewport.height
                visible: Boolean(modelData.clipModel) && width > 0
                    && x + width > 0 && x < layer.viewport.width
                color: modelData.problems[0].severity === DocumentProblems.Error ? Theme.errorColor : Theme.warningColor
                opacity: 0.125
            }
        }
    }
}
