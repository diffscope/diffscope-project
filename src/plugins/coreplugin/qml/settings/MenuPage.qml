import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

Item {
    id: page
    required property QtObject pageHandle
    property bool started: false

    required property QtObject model

    readonly property TextMatcher matcher: TextMatcher {}

    anchors.fill: parent

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        anchors.topMargin: 0
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            ToolBarContainer {
                id: toolBarContainer
                Instantiator {
                    model: toolActionsModel
                    onObjectAdded: (index, object) => {
                        if (object instanceof Action) {
                            toolBarContainer.addAction(object)
                        } else if (object instanceof Menu) {
                            toolBarContainer.addMenu(object)
                        }
                    }
                }
            }
            TextField {
                id: searchTextField
                ThemedItem.icon.source: "image://fluent-system-icon/search"
                placeholderText: qsTr("Search")
                Layout.fillWidth: true
            }
        }
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ActionLayoutsEditor {
                id: actionLayoutsEditor
                anchors.fill: parent
                anchors.margins: 1
                actionRegistry: CoreInterface.actionRegistry
                filterText: searchTextField.text
                model: page.model
            }
        }
    }

    ObjectModel {
        id: toolActionsModel
        Menu {
            title: qsTr("Add")
            icon.source: "image://fluent-system-icon/add_circle"
            Action {
                text: qsTr("Add Action or Menu...")
            }
            Action {
                text: qsTr("Add Separator")
                icon.source: "image://fluent-system-icons/line_horizontal_1_dashes"
            }
            Action {
                text: qsTr("Add Stretch")
                icon.source: "image://fluent-system-icons/auto_fit_width"
            }
        }
        Action {
            text: qsTr("Edit Icon...")
            icon.source: "image://fluent-system-icons/edit"
            shortcut: "F4"
        }
        Action {
            text: qsTr("Move Up")
            icon.source: "image://fluent-system-icons/arrow_circle_up"
            shortcut: "Alt+Up"
        }
        Action {
            text: qsTr("Move Down")
            icon.source: "image://fluent-system-icons/arrow_circle_down"
            shortcut: "Alt+Down"
        }
        Action {
            text: qsTr("Remove")
            icon.source: "image://fluent-system-icons/delete"
            shortcut: "Del"
        }
        Menu {
            title: qsTr("Restore")
            icon.source: "image://fluent-system-icons/arrow_hook_up_left"
            Action {
                text: qsTr("Restore This Menu")
            }
            Action {
                text: qsTr("Restore All")
            }
        }
    }
}