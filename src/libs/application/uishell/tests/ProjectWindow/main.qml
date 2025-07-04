import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml.Models

import DiffScope.UIShell

ProjectWindow {
    id: window
    visible: true
    icon: "qrc:/qt/qml/DiffScope/UIShell/Test/ProjectWindow/test_icon.png"
    title: "tst_uishell_ProjectWindow"
    component ListHelper: QtObject {
        default property list<QtObject> list: []
    }
    menusModel: ObjectModel {
        Menu {
            title: "&Test 1"
            Action { text: "aaa" }
            Action { text: "bbb" }
        }
        Menu {
            title: "T&est 2"
            Action { text: "aaa" }
            Action { text: "bbb" }
        }
    }
    toolButtonsModel: ObjectModel {
        ToolButton {
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        ToolButton {
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        ToolButton {
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
    }
    statusButtonsModel: ObjectModel {
        Label {
            text: "status text"
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        ToolButton {
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        ToolButton {
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
    }

    ListHelper {
        id: leftHelper
        DockingPane {
            title: "Test 1"
            iconSource: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        DockingStretch {
        }
    }
    leftDockingView.contentData: leftHelper.list

    ListHelper {
        id: rightHelper
        DockingPane {
            title: "Test 2"
            iconSource: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        DockingStretch {
        }
    }
    rightDockingView.contentData: rightHelper.list

    ListHelper {
        id: topHelper
        DockingPane {
            title: "Test 3"
            iconSource: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        DockingStretch {
        }
    }
    topDockingView.contentData: topHelper.list

    ListHelper {
        id: bottomHelper
        DockingPane {
            title: "Test 4"
            iconSource: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        DockingStretch {
        }
    }
    bottomDockingView.contentData: bottomHelper.list
}