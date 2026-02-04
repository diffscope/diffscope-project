import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

Dialog {
    id: dialog

    property int colorId

    title: qsTr("Pick Track Color")

    GridLayout {
        anchors.fill: parent
        columns: 6
        rowSpacing: 16
        columnSpacing: 16
        Repeater {
            model: CoreInterface.trackColorSchema.colors
            delegate: T.Button {
                id: control
                implicitWidth: 16
                implicitHeight: 16
                checkable: true
                checked: index === dialog.colorId
                autoExclusive: true
                required property color modelData
                required property int index
                text: qsTr("Track Color %L1").arg(index + 1)
                background: Rectangle {
                    color: modelData
                    border.color: control.checked ? Theme.foregroundPrimaryColor : Theme.borderColor
                    border.width: control.checked ? 2 : 1
                }
                onClicked: {
                    dialog.colorId = index
                }
            }
        }
    }
    standardButtons: DialogButtonBox.Ok
}