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

    component NavButton: Button {
        flat: true
        Layout.fillWidth: true
        checkable: true
        autoExclusive: true
        Component.onCompleted: contentItem.alignment = Qt.AlignLeft
    }
    component CellButton: Button {
        id: cell
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
                Frame {
                    anchors.fill: parent
                    ThemedItem.backgroundLevel: SVS.BL_Tertiary
                    ColorImage {
                        visible: cell.modelData.newFile === true
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
    }
    component ListItemButton: Button {
        id: cell
        required property var modelData
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
                    visible: cell.modelData.newFile === true
                    anchors.fill: parent
                    source: "qrc:/qt/qml/DiffScope/UIShell/assets/DocumentAdd48Regular.svg"
                    color: Theme.foregroundSecondaryColor
                }
                Image {
                    anchors.fill: parent
                    source: cell.modelData.icon
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
        StackLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
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
                    newFile: true,
                })
                ColumnLayout {
                    spacing: 16
                    anchors.fill: parent
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        TextField {
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
                    ScrollView {
                        id: fileGridScrollView
                        visible: !window.recentFilesListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        GridLayout {
                            rowSpacing: 16
                            columnSpacing: 16
                            width: parent.width
                            columns: Math.floor((fileGridScrollView.width - 4) / (160 + columnSpacing))
                            CellButton {
                                modelData: recentFiles.newFilePseudoElement
                            }
                            Repeater {
                                model: window.recentFilesModel
                                CellButton {

                                }
                            }
                        }
                    }
                    ScrollView {
                        id: fileListScrollView
                        visible: window.recentFilesListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        ColumnLayout {
                            visible: window.recentFilesListView
                            spacing: 4
                            width: parent.width
                            Repeater {
                                model: window.recentFilesModel
                                ListItemButton {
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }
            Pane {
                id: recoveryFiles
                ThemedItem.backgroundLevel: SVS.BL_Quaternary
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }
}