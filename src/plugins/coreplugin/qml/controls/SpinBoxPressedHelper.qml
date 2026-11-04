// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: d
    required property var spinBox
    property bool keyPressing: false
    property bool editing: false
    readonly property bool buttonPressed: d.spinBox.up.pressed || d.spinBox.down.pressed
    signal pressed()
    signal released()

    onButtonPressedChanged: updateEditing()
    onKeyPressingChanged: updateEditing()

    function updateEditing() {
        if (buttonPressed || keyPressing) {
            if (!editing) {
                editing = true
                pressed()
            }
        } else if (editing) {
            // A spin box can emit valueModified after releasing its button.
            Qt.callLater(finishEditing)
        }
    }

    function finishEditing() {
        if (!editing || buttonPressed || keyPressing)
            return
        editing = false
        released()
    }

    readonly property Connections focusConnections: Connections {
        target: d.spinBox
        function onActiveFocusChanged() {
            if (d.spinBox.activeFocus || !d.keyPressing)
                return
            d.keyPressing = false
            if (d.editing) {
                d.editing = false
                d.released()
            }
        }
    }
    readonly property Connections keysConnections: Connections {
        target: d.spinBox.Keys
        function onPressed(event) {
            event.accepted = false
            if (event.isAutoRepeat)
                return
            if (event.key === Qt.Key_Up || event.key === Qt.Key_Down)
                d.keyPressing = true
        }
        function onReleased(event) {
            event.accepted = false
            if (event.isAutoRepeat)
                return
            if (event.key === Qt.Key_Up || event.key === Qt.Key_Down)
                d.keyPressing = false
        }
    }
}
