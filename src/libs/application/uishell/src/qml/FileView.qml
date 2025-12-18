import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell

Item {
    id: view

    enum DisplayMode {
        Grid,
        List
    }

    property string searchText: ""
    property var filesModel: null
    property int displayMode: FileView.List
    property bool newFileActionEnabled: false
    property string emptyTip: ""

    readonly property var newFilePseudoElement: ({
        name: qsTr("New project"),
        path: "",
        lastModifiedText: "",
        thumbnail: "",
        icon: "",
    })

    signal newFileRequested()
    signal openFileRequested(int index)
    signal contextMenuRequested(int index)

    component CellButton: Button {
        id: cell
        required property int index
        required property var modelData
        flat: true
        padding: 4
        Accessible.name: modelData.name + "\n" + modelData.lastModifiedText
        Accessible.description: modelData.path
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
                    ColorImage {
                        visible: cell.index === -1
                        width: 80
                        height: 80
                        anchors.centerIn: parent
                        source: "image://fluent-system-icons/document_add?size=48&style=regular"
                        sourceSize.width: 80
                        sourceSize.height: 80
                        color: Theme.foregroundSecondaryColor
                    }
                    // fallback display icon as thumbnail
                    ColorImage {
                        width: 80
                        height: 80
                        anchors.centerIn: parent
                        source: cell.modelData.icon
                        color: cell.modelData.colorize ? Theme.foregroundSecondaryColor : "transparent"
                        sourceSize.width: 80
                        sourceSize.height: 80
                    }
                }
                Image {
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectCrop
                    source: cell.modelData.thumbnail
                    cache: false
                    mipmap: true
                }
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.width: 1
                    border.color: Theme.borderColor
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
        TapHandler {
            acceptedButtons: Qt.RightButton
            enabled: cell.index !== -1
            onSingleTapped: view.contextMenuRequested(filesProxyModel.mapIndexToSource(cell.index))
        }
        Keys.onMenuPressed: view.contextMenuRequested(filesProxyModel.mapIndexToSource(cell.index))
        onClicked: () => {
            if (cell.index === -1) {
                view.newFileRequested()
            } else {
                view.openFileRequested(filesProxyModel.mapIndexToSource(cell.index))
            }
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
        Accessible.description: modelData.path
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
                    source: "image://fluent-system-icons/document_add?size=48&style=regular"
                    color: Theme.foregroundSecondaryColor
                    sourceSize.width: 48
                    sourceSize.height: 48
                }
                ColorImage {
                    anchors.fill: parent
                    source: cell.modelData.icon
                    color: cell.modelData.colorize ? Theme.foregroundSecondaryColor : "transparent"
                    sourceSize.width: 48
                    sourceSize.height: 48
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
        TapHandler {
            acceptedButtons: Qt.RightButton
            enabled: cell.index !== -1
            onSingleTapped: view.contextMenuRequested(filesProxyModel.mapIndexToSource(cell.index))
        }
        Keys.onMenuPressed: view.contextMenuRequested(filesProxyModel.mapIndexToSource(cell.index))
        onClicked: () => {
            if (cell.index === -1) {
                view.newFileRequested()
            } else {
                view.openFileRequested(filesProxyModel.mapIndexToSource(cell.index))
            }
        }
    }

    RecentFilesProxyModel {
        id: filesProxyModel
        sourceModel: view.filesModel
        filterRole: USDef.RF_NameRole
        filterCaseSensitivity: Qt.CaseInsensitive
        property string _filterRegularExpression: searchTextField.text
        on_FilterRegularExpressionChanged: setFilterRegularExpression(_filterRegularExpression)
    }

    ColumnLayout {
        id: recentFilesLayout
        spacing: 16
        anchors.fill: parent
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: filesProxyModel.count === 0 && !(view.newFileActionEnabled && view.searchText.length === 0)
            Label {
                text: qsTr("No result found")
                ThemedItem.foregroundLevel: SVS.FL_Secondary
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: view.searchText.length !== 0
            }
            Label {
                text: view.emptyTip
                ThemedItem.foregroundLevel: SVS.FL_Secondary
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: view.searchText.length === 0
            }
        }
        ScrollView {
            id: fileGridScrollView
            visible: view.displayMode === FileView.Grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            GridLayout {
                id: fileGridLayout
                rowSpacing: 16
                columnSpacing: (fileGridScrollView.width - 172 * columns) / (columns - 1)
                width: parent.width
                columns: Math.floor((fileGridScrollView.width + 8) / (172 + 8))
                CellButton {
                    index: -1
                    modelData: view.newFilePseudoElement
                    visible: view.searchText.length === 0 && view.newFileActionEnabled
                }
                Repeater {
                    model: filesProxyModel
                    CellButton {
                    }
                }
            }
        }
        ScrollView {
            id: fileListScrollView
            visible: view.displayMode === FileView.List
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
                    modelData: view.newFilePseudoElement
                    visible: view.searchText.length === 0 && view.newFileActionEnabled
                }
                Repeater {
                    model: filesProxyModel
                    ListItemButton {
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}