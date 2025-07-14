import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

Window {
    id: window
    visible: true
    width: 800
    height: 600
    required property QtObject model
    Rectangle {
        color: Theme.backgroundQuaternaryColor
        anchors.fill: parent
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            Switch {
                id: autoWrapSwitch
                text: "Auto wrap"
            }
            LyricPronunciationEditor {
                autoWrap: autoWrapSwitch.checked
                model: window.model
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}