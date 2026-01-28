import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: d
    required property Slider slider
    property bool keyPressing: false
    signal pressed()
    signal released()
    readonly property Connections sliderConnections: Connections {
        target: d.slider
        function onPressedChanged() {
            if (d.keyPressing)
                return
            if (d.slider.pressed)
                d.pressed()
            else
                d.released()
        }
    }
    readonly property Connections keysConnections: Connections {
        target: d.slider.Keys
        function onPressed(event) {
            event.accepted = false
            if (event.isAutoRepeat)
                return
            if (event.key === Qt.Key_Left || event.key === Qt.Key_Right) {
                d.keyPressing = true
                d.pressed()
            }
        }
        function onReleased (event) {
            event.accepted = false
            if (event.isAutoRepeat)
                return
            if (event.key === Qt.Key_Left || event.key === Qt.Key_Right) {
                d.keyPressing = false
                d.released()
            }
        }
    }
}