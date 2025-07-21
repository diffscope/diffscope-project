import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml.Models

import SVSCraft

import DiffScope.UIShell

ProjectWindow {
    id: window
    visible: true
    icon: "qrc:/qt/qml/DiffScope/UIShell/Test/ProjectWindow/test_icon.png"
    documentName: "test.dspx"
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
        Action {
            text: "Action 1"
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Action {
            text: "Checkable Action 2"
            checkable: true
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
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

    property Component bubbleNotificationHandle: QtObject {
        property string title: "111"
        property string text: "222"
        property int icon: SVS.NoIcon
        property list<string> buttons: []
        property int primaryButton: 0
        property bool closable: true
        property bool hasProgress: false
        property double progress: 0
        property bool progressAbortable: false
        property bool permanentlyHideable: true
        property int textFormat: Text.AutoText
        function hideClicked() {
            bnm.remove(ObjectModel.index)
        }
        function hoverEntered() {
            console.log("hover entered")
        }
    }

    bubbleNotificationsModel: ObjectModel {
        id: bnm
        QtObject {
            property string title: "Test"
            property string text: 'This is a test <a href="aaa">link</a>'
            property int icon: SVS.Information
            property list<string> buttons: ["AAA", "BBB"]
            property int primaryButton: 1
            property bool closable: true
            property bool hasProgress: true
            property double progress: -1
            property bool progressAbortable: true
            property bool permanentlyHideable: true
            function buttonClicked (index) {
                bnm.append(window.bubbleNotificationHandle.createObject())
            }
            function linkActivated (link) {
                console.log(link)
            }
            signal hoverEntered()
        }
        QtObject {
            property string title: "Error"
            property string text: ""
            property int icon: SVS.Critical
            property list<string> buttons: []
            property int primaryButton: 0
            property bool closable: true
            property bool hasProgress: false
            property double progress: 0
            property bool progressAbortable: false
            property bool permanentlyHideable: true
        }
        QtObject {
            property string title: "Warning"
            property string text: "aaa"
            property int icon: SVS.Warning
            property list<string> buttons: ["Yes", "No"]
            property int primaryButton: 0
            property bool closable: false
            property bool hasProgress: false
            property double progress: 0
            property bool progressAbortable: false
            property bool permanentlyHideable: false
        }
    }
}