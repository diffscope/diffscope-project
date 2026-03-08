import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

DockingPane {
    id: panel

    required property string actionId
    required property string actionText
    required property bool componentRegistered
    required property string errorString

    title: actionText + " " + "(Failed to load)"
    icon.source: "image://fluent-system-icons/error_circle"
    icon.width: 16
    icon.height: 16
    icon.color: Theme.errorColor

    property var state
    function loadState(s) {
        state = s
    }

    function saveState() {
        return state
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 12
        spacing: 12
        IconLabel {
            text: qsTr("Failed to load component")
            color: Theme.foregroundPrimaryColor
            font.pixelSize: 16
            icon.source: "image://fluent-system-icons/error_circle"
            icon.width: 48
            icon.height: 48
            icon.color: Theme.errorColor
            spacing: 12
            Accessible.role: Accessible.staticText
            Accessible.name: qsTr("Failed to load component")
        }
        Label {
            visible: !panel.componentRegistered
            text: qsTr("This component cannot be loaded because it is not registered to the application. The plugin providing this component might be disabled or no longer available.")
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }
        Label {
            visible: panel.componentRegistered
            text: qsTr("An error occurred while loading this component.")
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }
        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Component identifier")
            columnItem: TextField {
                text: panel.actionId
                readOnly: true
            }
        }
        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Error")
            visible: panel.errorString !== ""
            columnItem: TextArea {
                text: panel.errorString
                readOnly: true
                wrapMode: Text.Wrap
            }
        }
    }
}