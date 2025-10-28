import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

ActionDockingPane {
    header: RowLayout {
        anchors.fill: parent
        spacing: 4
        ToolButton {
            icon.source: "qrc:/diffscope/coreplugin/icons/AddCircle16Filled.svg"
            text: qsTr("Add Track")
        }
        Rectangle {
            width: 1
            Layout.fillHeight: true
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            color: Theme.borderColor
        }
        ButtonGroup {
            id: editModeButtonGroup
            exclusive: true
        }
        ToolButton {
            icon.source: "qrc:/diffscope/coreplugin/icons/Cursor16Filled.svg"
            text: qsTr("Pointer")
            display: AbstractButton.IconOnly
            checkable: true
            checked: true
            ButtonGroup.group: editModeButtonGroup
        }
        ToolButton {
            icon.source: "qrc:/diffscope/coreplugin/icons/Edit16Filled.svg"
            text: qsTr("Pen")
            display: AbstractButton.IconOnly
            checkable: true
            ButtonGroup.group: editModeButtonGroup
        }
        ToolButton {
            icon.source: "qrc:/diffscope/coreplugin/icons/Cut16Filled.svg"
            text: qsTr("Scissor")
            display: AbstractButton.IconOnly
            checkable: true
            ButtonGroup.group: editModeButtonGroup
        }
        Item {
            Layout.fillWidth: true
        }
        ToolButton {
            text: qsTr("Toggle Tempo Track")
            display: AbstractButton.IconOnly
            checkable: true
        }
        ToolButton {
            icon.source: "qrc:/diffscope/coreplugin/icons/Tag16Filled"
            text: qsTr("Toggle Tag Track")
            display: AbstractButton.IconOnly
            checkable: true
        }
    }
}