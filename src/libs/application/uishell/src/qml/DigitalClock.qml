import QtQml
import QtQuick
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents

T.Button {
    id: control
    property bool backgroundVisible: true
    
    implicitWidth: Math.max(backgroundVisible ? 108 : 0, timeLabel.width + 16)
    implicitHeight: 24
    Accessible.name: qsTr("Digital clock")
    Accessible.description: text
    hoverEnabled: true

    background: Rectangle {
        visible: control.backgroundVisible
        color: Theme.backgroundTertiaryColor
        border.color: Theme.borderColor
        radius: 4
    }

    DigitalClockLabel {
        id: timeLabel
        text: control.text
        anchors.centerIn: parent
        color: Theme.foregroundPrimaryColor
        font.pixelSize: 16
    }
}