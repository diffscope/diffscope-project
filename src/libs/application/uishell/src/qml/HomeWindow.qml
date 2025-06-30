import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

ApplicationWindow {
    id: window
    width: 800
    height: 500
    property string banner: ""
    property ListModel recentFilesModel: null
    property bool recentFilesListView: false
    property ListModel recoveryFilesModel: null

    component NavButton: Button {
        flat: true
        Layout.fillWidth: true
        checkable: true
        autoExclusive: true
        Component.onCompleted: contentItem.alignment = Qt.AlignLeft
    }
    component CellButton: Button {
        id: cell
        required property int index
        required property var modelData
        flat: true
        padding: 4
        Accessible.name: modelData.name + "\n" + modelData.lastModifiedText
        DescriptiveText.toolTip: modelData.path
        DescriptiveText.activated: !modelData.newFile && hovered
        contentItem: ColumnLayout {
            id: cellContent
            spacing: 4
            Item {
                implicitWidth: 160
                implicitHeight: 120
                Layout.alignment: Qt.AlignHCenter
                Rectangle {
                    anchors.fill: parent
                    color: Theme.backgroundTertiaryColor
                    border.width: 1
                    border.color: Theme.borderColor
                    ColorImage {
                        visible: cell.index === -1
                        width: 80
                        height: 80
                        anchors.centerIn: parent
                        source: "qrc:/qt/qml/DiffScope/UIShell/assets/DocumentAdd48Regular.svg"
                        color: Theme.foregroundSecondaryColor
                    }
                    // fallback display icon as thumbnail
                    Image {
                        width: 80
                        height: 80
                        anchors.centerIn: parent
                        source: cell.modelData.icon
                    }
                }
                Image {
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectCrop
                    source: cell.modelData.thumbnail
                }
            }
            Label {
                id: nameLabel
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 160
                text: cell.modelData.name
                elide: Text.ElideMiddle
            }
            Label {
                id: lastModifiedTextLabel
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 160
                text: cell.modelData.lastModifiedText
                elide: Text.ElideMiddle
                ThemedItem.foregroundLevel: SVS.FL_Secondary
            }
        }
        FileMenuHandler {
            index: cell.index
            modelData: cell.modelData
        }
    }
    component ListItemButton: Button {
        id: cell
        required property int index
        required property var modelData
        property bool recovery: false
        flat: true
        padding: 4
        Accessible.name: modelData.name + "\n" + modelData.lastModifiedText
        DescriptiveText.toolTip: modelData.path
        DescriptiveText.activated: !modelData.newFile && hovered
        contentItem: RowLayout {
            id: cellContent
            spacing: 4
            Item {
                implicitWidth: 48
                implicitHeight: 48
                Layout.alignment: Qt.AlignHCenter
                ColorImage {
                    visible: cell.index === -1
                    anchors.fill: parent
                    source: "qrc:/qt/qml/DiffScope/UIShell/assets/DocumentAdd48Regular.svg"
                    color: Theme.foregroundSecondaryColor
                }
                Image {
                    anchors.fill: parent
                    source: cell.modelData.icon
                }
            }
            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true
                RowLayout {
                    spacing: 4
                    Label {
                        Layout.fillWidth: true
                        text: cell.modelData.name
                        elide: Text.ElideMiddle
                    }
                    Label {
                        text: cell.modelData.lastModifiedText
                        elide: Text.ElideMiddle
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                    }
                }
                Label {
                    visible: cell.modelData.path.length !== 0
                    text: cell.modelData.path
                    elide: Text.ElideMiddle
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                }
            }
        }
        FileMenuHandler {
            index: cell.index
            modelData: cell.modelData
            recovery: cell.recovery
        }
    }
    component FileMenuHandler: TapHandler {
        id: tapHandler
        required property int index
        required property var modelData
        property bool recovery: false
        readonly property Menu fileMenu: Menu {
            Action {
                text: qsTr("Open")
                icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/FolderOpen16Filled.svg"
            }
            Action {
                text: qsTr("Open File Location")
                icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/OpenFolder16Filled.svg"
                enabled: tapHandler.modelData.path.length !== 0
            }
            Action {
                text: tapHandler.recovery ? qsTr('Remove from "Recovery Files"') : qsTr('Remove from "Recent Files"')
                icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/DocumentDismiss16Filled.svg"
            }
        }
        acceptedButtons: Qt.RightButton
        enabled: index !== -1
        onSingleTapped: () => {
            fileMenu.popup()
        }
    }

    RowLayout {
        spacing: 0
        anchors.fill: parent
        Pane {
            id: nav
            padding: 6
            contentWidth: 200
            Layout.fillHeight: true
            ColumnLayout {
                width: 200
                height: parent.height
                spacing: 6
                Item {
                    Layout.fillWidth: true
                    implicitHeight: banner.implicitHeight * width / banner.implicitWidth
                    Image {
                        id: banner
                        anchors.fill: parent
                        source: window.banner
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    NavButton {
                        text: qsTr("Recent Files")
                        checked: true
                        icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/History16Filled.svg"
                    }
                    NavButton {
                        id: recoveryFilesButton
                        text: qsTr("Recovery Files")
                        icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/DocumentSync16Filled.svg"
                        Rectangle {
                            width: Math.max(16, recoveryFileCountText.width)
                            height: 16
                            radius: 8
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 6
                            color: Theme.warningColor
                            Label {
                                id: recoveryFileCountText
                                padding: 2
                                anchors.centerIn: parent
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 10
                                text: "5"
                            }
                        }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.borderColor
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
        Pane {
            id: recentFiles
            ThemedItem.backgroundLevel: SVS.BL_Quaternary
            Layout.fillHeight: true
            Layout.fillWidth: true
            padding: 16
            readonly property var newFilePseudoElement: ({
                name: qsTr("New File"),
                path: "",
                lastModifiedText: "",
                thumbnail: "",
                icon: "",
            })
            ColumnLayout {
                spacing: 16
                anchors.fill: parent
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    TextField {
                        id: searchTextField
                        placeholderText: qsTr("Search")
                        Layout.fillWidth: true
                        leftPadding: 32
                        ColorImage {
                            width: 16
                            height: 16
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            source: "qrc:/qt/qml/DiffScope/UIShell/assets/Search16Filled.svg"
                            color: Theme.foregroundPrimaryColor
                        }
                    }
                    RowLayout {
                        visible: !recoveryFilesButton.checked
                        ToolButton {
                            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
                            checkable: true
                            autoExclusive: true
                            checked: !window.recentFilesListView
                            onClicked: GlobalHelper.setProperty(window, "recentFilesListView", !checked)
                            DescriptiveText.toolTip: qsTr("Grid view")
                            DescriptiveText.activated: hovered
                        }
                        ToolButton {
                            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/List16Filled.svg"
                            checkable: true
                            autoExclusive: true
                            checked: window.recentFilesListView
                            onClicked: GlobalHelper.setProperty(window, "recentFilesListView", checked)
                            DescriptiveText.toolTip: qsTr("List view")
                            DescriptiveText.activated: hovered
                        }
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Label {
                        text: qsTr("No result found")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: searchTextField.length !== 0 && (fileGridLayout.visibleChildren.length === 1 || fileListLayout.visibleChildren.length === 1)
                    }
                }
                ScrollView {
                    id: fileGridScrollView
                    visible: !window.recentFilesListView && !recoveryFilesButton.checked
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    GridLayout {
                        id: fileGridLayout
                        rowSpacing: 16
                        columnSpacing: 16
                        width: parent.width
                        columns: Math.floor(fileGridScrollView.width / (160 + columnSpacing))
                        CellButton {
                            index: -1
                            modelData: recentFiles.newFilePseudoElement
                            visible: searchTextField.text.length === 0
                        }
                        Repeater {
                            model: window.recentFilesModel
                            CellButton {
                                visible: modelData.name.toLowerCase().indexOf(searchTextField.text.toLowerCase()) !== -1
                            }
                        }
                    }
                }
                ScrollView {
                    id: fileListScrollView
                    visible: window.recentFilesListView || recoveryFilesButton.checked
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout {
                        id: fileListLayout
                        spacing: 4
                        implicitWidth: fileListScrollView.width
                        width: fileListScrollView.width
                        ListItemButton {
                            Layout.fillWidth: true
                            index: -1
                            modelData: recentFiles.newFilePseudoElement
                            visible: searchTextField.text.length === 0 && !recoveryFilesButton.checked
                        }
                        Repeater {
                            model: recoveryFilesButton.checked ? window.recoveryFilesModel : window.recentFilesModel
                            ListItemButton {
                                Layout.fillWidth: true
                                visible: modelData.name.toLowerCase().indexOf(searchTextField.text.toLowerCase()) !== -1
                                recovery: recoveryFilesButton.checked
                            }
                        }
                    }
                }
            }
        }
    }
}