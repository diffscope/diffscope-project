import QtQml
import QtQuick

import DiffScope.UIShell

HomeWindow {
    id: window
    visible: true
    banner: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png"
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
}