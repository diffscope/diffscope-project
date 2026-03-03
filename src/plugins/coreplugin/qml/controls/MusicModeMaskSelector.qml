import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

Item {
    id: control
    implicitWidth: 200
    implicitHeight: 60

    property int mode: 0
    property int tonality: 0
    signal modeModified()
    signal tonalityModified()

    readonly property int cMask: SVS.musicMode(mode).translateMask(tonality, 0)

    ListModel {
        id: pianoKeyModel
        ListElement {
            name: "C"
            x: 0
            width: 0.14285714285714285
            height: 1
        }
        ListElement {
            name: "C\u266f/D\u266d"
            x: 0.09285714285714285
            width: 0.1
            height: 0.6666666666666666
        }
        ListElement {
            name: "D"
            x: 0.14285714285714285
            width: 0.14285714285714285
            height: 1
        }
        ListElement {
            name: "D\u266f/E\u266d"
            x: 0.2357142857142857
            width: 0.1
            height: 0.6666666666666666
        }
        ListElement {
            name: "E"
            x: 0.2857142857142857
            width: 0.14285714285714285
            height: 1
        }
        ListElement {
            name: "F"
            x: 0.42857142857142855
            width: 0.14285714285714285
            height: 1
        }
        ListElement {
            name: "F\u266f/G\u266d"
            x: 0.5214285714285714
            width: 0.1
            height: 0.6666666666666666
        }
        ListElement {
            name: "G"
            x: 0.5714285714285714
            width: 0.14285714285714285
            height: 1
        }
        ListElement {
            name: "G\u266f/A\u266d"
            x: 0.6642857142857143
            width: 0.1
            height: 0.6666666666666666
        }
        ListElement {
            name: "A"
            x: 0.7142857142857143
            width: 0.14285714285714285
            height: 1
        }
        ListElement {
            name: "A\u266f/B\u266d"
            x: 0.807142857142857
            width: 0.1
            height: 0.6666666666666666
        }
        ListElement {
            name: "B"
            x: 0.8571428571428571
            width: 0.14285714285714285
            height: 1
        }
    }

    Repeater {
        model: pianoKeyModel
        delegate: T.Button {
            id: button
            required property int index
            required property var modelData
            x: modelData.x * control.width
            width: modelData.width * control.width
            height: modelData.height * control.height
            z: [0, 2, 4, 5, 7, 9, 11].includes(button.index) ? 0 : 1
            checkable: true
            checked: Boolean(control.cMask & (1 << index))
            text: modelData.name
            background: Rectangle {
                color: [0, 2, 4, 5, 7, 9, 11].includes(button.index) ? "#f8f9fa" : "#212529"
                border.color: Theme.borderColor
                border.width: 1
                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 8
                    color: button.index === control.tonality ? Theme.accentColor : [0, 2, 4, 5, 7, 9, 11].includes(button.index) ? "#252525" : "#dadada"
                    visible: button.checked
                }
                bottomLeftRadius: 4
                bottomRightRadius: 4
            }
            onClicked: () => {
                let newCMask = control.cMask
                if (button.checked || button.index === control.tonality) {
                    newCMask |= (1 << button.index)
                } else {
                    newCMask &= ~(1 << button.index)
                }
                let newMode = SVS.musicMode(newCMask).translateMask(0, control.tonality)
                if (newMode !== control.mode) {
                    GlobalHelper.setProperty(control, "mode", newMode)
                    control.modeModified()
                }
                if (button.index === control.tonality) {
                    GlobalHelper.setProperty(button, "checked", true)
                }
            }
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: () => {
                    let newCMask = control.cMask
                    newCMask |= (1 << button.index)
                    GlobalHelper.setProperty(control, "tonality", button.index)
                    control.tonalityModified()
                    let newMode = SVS.musicMode(newCMask).translateMask(0, control.tonality)
                    if (newMode !== control.mode) {
                        GlobalHelper.setProperty(control, "mode", newMode)
                        control.modeModified()
                    }
                }
            }
        }
    }
}