import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: d
    required property SpinBox spinBox
    property bool keyPressing: false
    signal pressed()
    signal released()
    readonly property bool buttonPressed: d.spinBox.up.pressed || d.spinBox.down.pressed
    readonly property Connections upConnections: Connections {
        target: d.spinBox.up
        function onPressedChanged() {
            if (d.keyPressing)
                return
            if (d.spinBox.up.pressed)
                d.pressed()
            else
                d.released()
        }
    }
    readonly property Connections downConnections: Connections {
        target: d.spinBox.down
        function onPressedChanged() {
            if (d.keyPressing)
                return
            if (d.spinBox.down.pressed)
                d.pressed()
            else
                d.released()
        }
    }
    readonly property Connections keysConnections: Connections {
        target: d.spinBox.Keys
        function onPressed(event) {
            event.accepted = false
            if (event.isAutoRepeat)
                return
            if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
                d.keyPressing = true
                d.pressed()
            }
        }
        function onReleased (event) {
            event.accepted = false
            if (event.isAutoRepeat)
                return
            if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
                d.keyPressing = false
                d.released()
            }
        }
    }
}