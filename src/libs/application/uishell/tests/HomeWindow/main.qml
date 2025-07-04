import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import DiffScope.UIShell

HomeWindow {
    id: window
    visible: true
    banner: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_logo.png"
    recentFilesModel: ListModel {
        ListElement {
            name: "File 1"
            path: "/path/to/file1.dspx"
            lastModifiedText: "11:45 Today"
            thumbnail: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
        ListElement {
            name: "File Without Thumbnail"
            path: "/path/to/file1.dspx"
            lastModifiedText: "11:45 Today"
            thumbnail: ""
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
        ListElement {
            name: "Very Long Very Long Very Long Very Long"
            path: "/path/to/file1.dspx"
            lastModifiedText: "Very Long Very Long Very Long Very Long"
            thumbnail: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
        ListElement {
            name: "File 2"
            path: "/path/to/file1.dspx"
            lastModifiedText: "11:45 Today"
            thumbnail: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
        ListElement {
            name: "File 3"
            path: "/path/to/file1.dspx"
            lastModifiedText: "11:45 Today"
            thumbnail: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
        ListElement {
            name: "File 4"
            path: "/path/to/file1.dspx"
            lastModifiedText: "11:45 Today"
            thumbnail: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
        ListElement {
            name: "File 5"
            path: "/path/to/file1.dspx"
            lastModifiedText: "11:45 Today"
            thumbnail: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
        ListElement {
            name: "File 6"
            path: "/path/to/file1.dspx"
            lastModifiedText: "11:45 Today"
            thumbnail: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
        ListElement {
            name: "File 7"
            path: "/path/to/file1.dspx"
            lastModifiedText: "11:45 Today"
            thumbnail: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
        ListElement {
            name: "File 8"
            path: "/path/to/file1.dspx"
            lastModifiedText: "11:45 Today"
            thumbnail: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
    }
    recoveryFilesModel: ListModel {
        ListElement {
            name: "Recovery File"
            path: "/path/to/file1.dspx"
            lastModifiedText: "11:45 Today"
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
        ListElement {
            name: "Unsaved File"
            path: ""
            lastModifiedText: "11:45 Today"
            icon: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
        }
    }
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