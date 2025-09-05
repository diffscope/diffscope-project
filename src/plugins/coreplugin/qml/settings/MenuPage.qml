import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

Item {
    id: page
    required property QtObject pageHandle
    property bool started: false

    required property QtObject model

    readonly property TextMatcher matcher: TextMatcher {}

    anchors.fill: parent

    ObjectModel {
        id: toolActionsModel
        Menu {
            title: qsTr("Add")
            icon.source: "qrc:/diffscope/coreplugin/icons/AddCircle16Filled.svg"
            Action {
                text: qsTr("Add Action or Menu...")
            }
            Action {
                text: qsTr("Add Separator")
                icon.source: "qrc:/diffscope/coreplugin/icons/LineHorizontal1Dashed16Filled.svg"
            }
            Action {
                text: qsTr("Add Stretch")
                icon.source: "qrc:/diffscope/coreplugin/icons/AutoFitWidth20Filled.svg"
            }
        }
        Action {
            text: qsTr("Edit Icon...")
            icon.source: "qrc:/diffscope/coreplugin/icons/Edit16Filled.svg"
            shortcut: "F4"
        }
        Action {
            text: qsTr("Move Up")
            icon.source: "qrc:/diffscope/coreplugin/icons/ArrowCircleUp16Filled.svg"
            shortcut: "Alt+Up"
        }
        Action {
            text: qsTr("Move Down")
            icon.source: "qrc:/diffscope/coreplugin/icons/ArrowCircleDown16Filled.svg"
            shortcut: "Alt+Down"
        }
        Action {
            text: qsTr("Remove")
            icon.source: "qrc:/diffscope/coreplugin/icons/Delete16Filled.svg"
            shortcut: "Del"
        }
        Menu {
            title: qsTr("Restore")
            icon.source: "qrc:/diffscope/coreplugin/icons/ArrowHookUpLeft16Filled.svg"
            Action {
                text: qsTr("Restore This Menu")
            }
            Action {
                text: qsTr("Restore All")
            }
        }
    }

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
                id: searchTextFiled
                ThemedItem.icon.source: "qrc:/diffscope/coreplugin/icons/Search16Filled.svg"
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
                model: page.model
            }
        }
    }
}