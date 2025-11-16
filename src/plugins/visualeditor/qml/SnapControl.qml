import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Item {
    id: control

    property PositionAlignmentManipulator positionAlignmentManipulator: null

    implicitWidth: layout.implicitWidth
    implicitHeight: layout.implicitHeight

    RowLayout {
        id: layout
        Label {
            text: qsTr("Snap")
        }
        ComboBox {
            implicitHeight: 24

            model: [
                { text: qsTr("Auto"), data: PositionAlignmentManipulator.Auto },
                { text: qsTr("None"), data: PositionAlignmentManipulator.Unset },
                { text: qsTr("Whole note"), data: PositionAlignmentManipulator.Note1st },
                { text: qsTr("Half note"), data: PositionAlignmentManipulator.Note2nd },
                { text: qsTr("Quarter note"), data: PositionAlignmentManipulator.Note4th },
                { text: qsTr("8th note"), data: PositionAlignmentManipulator.Note8th },
                { text: qsTr("16th note"), data: PositionAlignmentManipulator.Note16th },
                { text: qsTr("32nd note"), data: PositionAlignmentManipulator.Note32nd },
                { text: qsTr("64th note"), data: PositionAlignmentManipulator.Note64th },
                { text: qsTr("128th note"), data: PositionAlignmentManipulator.Note128th },
            ]
            textRole: "text"
            valueRole: "data"
            Component.onCompleted: currentIndex = Qt.binding(() => indexOfValue(control.positionAlignmentManipulator?.duration ?? 0))
            onCurrentValueChanged: () => {
                if (control.positionAlignmentManipulator) {
                    control.positionAlignmentManipulator.duration = currentValue
                }
            }
        }
        ToolButton {
            text: qsTr("Triplet")
            display: AbstractButton.IconOnly
            icon.source: "qrc:/diffscope/visualeditor/icons/triplet.svg"
            enabled: control.positionAlignmentManipulator?.duration !== PositionAlignmentManipulator.Unset
            checkable: true
            checked: control.positionAlignmentManipulator?.tuplet === PositionAlignmentManipulator.Triplet
            onClicked: () => {
                if (control.positionAlignmentManipulator) {
                    control.positionAlignmentManipulator.tuplet = checked ? PositionAlignmentManipulator.Triplet : PositionAlignmentManipulator.None
                }
            }
        }
        ToolButton {
            text: qsTr("Quintuplet")
            display: AbstractButton.IconOnly
            icon.source: "qrc:/diffscope/visualeditor/icons/quintuplet.svg"
            enabled: control.positionAlignmentManipulator?.duration !== PositionAlignmentManipulator.Unset
            checkable: true
            checked: control.positionAlignmentManipulator?.tuplet === PositionAlignmentManipulator.Quintuplet
            onClicked: () => {
                if (control.positionAlignmentManipulator) {
                    control.positionAlignmentManipulator.tuplet = checked ? PositionAlignmentManipulator.Quintuplet : PositionAlignmentManipulator.None
                }
            }
        }
    }
}