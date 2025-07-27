import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import DiffScope.UIShell

HomeWindow {
    id: window
    visible: true
    banner: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_logo.png"
    navigationActionsModel: ObjectModel {
        Action {
            text: "New"
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Action {
            text: "Open..."
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Action {
            text: "Import..."
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Action {
            text: "Custom Action"
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Menu {
            title: "Custom Menu"
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
            Action {
                text: "aaa"
            }
            Action {
                text: "bbb"
            }
        }
    }
    toolActionsModel: ObjectModel {
        Action {
            text: "Settings..."
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Action {
            text: "Plugins..."
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Menu {
            title: "Help"
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
            Action { text: "Help" }
            Action { text: "About" }
        }
    }

    onNewFileRequested: console.log("new file")
    onOpenRecentFileRequested: (index) => {
        console.log("open recent file", index)
    }
    onOpenRecoveryFileRequested: (index) => {
        console.log("open recovery file", index)
    }
}