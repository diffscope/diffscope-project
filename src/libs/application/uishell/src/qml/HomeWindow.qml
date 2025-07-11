import QtQml
import QtQml.Models
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
    property bool frameless: true
    property url banner: ""
    property var recentFilesModel: null
    property bool recentFilesListView: false
    property var recoveryFilesModel: null
    property var navigationActionsModel: null
    property var toolActionsModel: null

    signal newFileRequested()
    signal openRecentFileRequested(int index)
    signal openRecoveryFileRequested(int index)

    function setupFrameless() {
        if (frameless && !windowAgent.framelessSetup) {
            windowAgent.setup(window)
            windowAgent.framelessSetup = true
            windowAgent.setTitleBar(titleBarArea)
            windowAgent.setSystemButton(WindowAgent.Minimize, minimizeSystemButton)
            windowAgent.setSystemButton(WindowAgent.Maximize, maximizeSystemButton)
            windowAgent.setSystemButton(WindowAgent.Close, closeSystemButton)
        }
    }

    Component.onCompleted: () => {
        setupFrameless()
    }

    onFramelessChanged: () => {
        setupFrameless()
    }


    component NavButton: Button {
        flat: true
        Layout.fillWidth: true
        Component.onCompleted: contentItem.alignment = Qt.AlignLeft
    }
    component NavToolButton: Button {
        flat: true
        display: AbstractButton.IconOnly
        implicitWidth: implicitHeight
        DescriptiveText.toolTip: text
        DescriptiveText.activated: hovered
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
            id: fileMenuHandler
            index: cell.index
            modelData: cell.modelData
        }
        Keys.onMenuPressed: fileMenuHandler.fileMenu.popup(this, width / 2, height / 2)
        onClicked: () => {
            if (cell.index === -1) {
                window.newFileRequested()
            } else if (cell.recovery) {
                window.openRecoveryFileRequested(cell.index)
            } else {
                window.openRecentFileRequested(cell.index)
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
            id: fileMenuHandler
            index: cell.index
            modelData: cell.modelData
            recovery: cell.recovery
        }
        Keys.onMenuPressed: fileMenuHandler.fileMenu.popup(this, width / 2, height / 2)
        onClicked: () => {
            if (cell.index === -1) {
                window.newFileRequested()
            } else if (cell.recovery) {
                window.openRecoveryFileRequested(cell.index)
            } else {
                window.openRecentFileRequested(cell.index)
            }
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
                onTriggered: () => {
                    if (tapHandler.recovery) {
                        window.openRecoveryFileRequested(tapHandler.index)
                    } else {
                        window.openRecentFileRequested(tapHandler.index)
                    }
                }
            }
            Action {
                text: qsTr("Open File Location")
                icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/OpenFolder16Filled.svg"
                enabled: tapHandler.modelData.path.length !== 0
            }
            Action {
                text: tapHandler.recovery ? qsTr('Remove from "Recovery Files"') : qsTr('Remove from "Recent Files"')
                icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/DocumentDismiss16Filled.svg"
                onTriggered: () => {
                    const model = tapHandler.recovery ? window.recoveryFilesModel : window.recentFilesModel
                    model.remove(tapHandler.index)
                }
            }
        }
        acceptedButtons: Qt.RightButton
        enabled: index !== -1
        onSingleTapped: () => {
            fileMenu.popup()
        }
    }

    WindowAgent {
        id: windowAgent
        property bool framelessSetup: false
    }

    Item {
        id: titleBarArea
        width: window.width
        height: Qt.platform.os !== "osx" && Qt.platform.os !== "macos" ? 36 : 28
        visible: windowAgent.framelessSetup
        z: 1
        Accessible.role: Accessible.TitleBar
        RowLayout {
            anchors.right: parent.right
            visible: Qt.platform.os !== "osx" && Qt.platform.os !== "macos"
            spacing: 0
            SystemButton {
                id: minimizeSystemButton
                type: SystemButton.Minimize
            }
            SystemButton {
                id: maximizeSystemButton
                type: SystemButton.MaximizeRestore
            }
            SystemButton {
                id: closeSystemButton
                type: SystemButton.Close
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
                id: navLayout
                width: 200
                height: parent.height
                spacing: 6
                Item {
                    Layout.fillWidth: true
                    height: titleBarArea.height - nav.topPadding - navLayout.spacing
                }
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
                        Accessible.role: Accessible.RadioButton
                        checkable: true
                        autoExclusive: true
                        checked: true
                        icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/History16Filled.svg"
                    }
                    NavButton {
                        id: recoveryFilesButton
                        text: qsTr("Recovery Files")
                        Accessible.role: Accessible.RadioButton
                        checkable: true
                        autoExclusive: true
                        icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/DocumentSync16Filled.svg"
                        Rectangle {
                            width: Math.max(16, recoveryFileCountText.width)
                            height: 16
                            radius: 8
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 6
                            color: Theme.warningColor
                            visible: (window.recoveryFilesModel?.count ?? 0) !== 0
                            Label {
                                id: recoveryFileCountText
                                padding: 2
                                anchors.centerIn: parent
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 10
                                text: (window.recoveryFilesModel?.count ?? 0).toLocaleString()
                            }
                        }
                    }
                }
                MenuSeparator {
                    Layout.fillWidth: true
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Repeater {
                        model: ObjectModel {
                            id: navigationActionButtonsModel
                            readonly property Instantiator instantiator: Instantiator {
                                model: window.navigationActionsModel
                                readonly property Component navButton: NavButton {
                                    id: button
                                    property Menu submenu: null
                                    action: MenuAction {
                                        menu: button.submenu
                                        parentItem: button
                                    }
                                }
                                onObjectAdded: (index, object) => {
                                    if (object instanceof Action) {
                                        navigationActionButtonsModel.insert(index, navButton.createObject(null, {
                                            action: object
                                        }))
                                    } else if (object instanceof Menu) {
                                        navigationActionButtonsModel.insert(index, navButton.createObject(null, {
                                            submenu: object
                                        }))
                                    } else {
                                        navigationActionButtonsModel.insert(index, navButton.createObject(null, {
                                            visible: false
                                        }))
                                    }

                                }
                                onObjectRemoved: (index, object) => {
                                    const o = navigationActionButtonsModel.get(o)
                                    navigationActionButtonsModel.remove(index)
                                    o.destroy()
                                }
                            }
                        }
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                RowLayout {
                    Layout.leftMargin: 2
                    spacing: 6
                    Repeater {
                        model: ObjectModel {
                            id: toolActionButtonsModel
                            readonly property Instantiator instantiator: Instantiator {
                                model: window.toolActionsModel
                                readonly property Component toolButton: NavToolButton {
                                    id: button
                                    property Menu submenu: null
                                    action: MenuAction {
                                        menu: button.submenu
                                        parentItem: button
                                        menuDisplay: MenuAction.Top
                                    }
                                }
                                onObjectAdded: (index, object) => {
                                    if (object instanceof Action) {
                                        toolActionButtonsModel.insert(index, toolButton.createObject(null, {
                                            action: object
                                        }))
                                    } else if (object instanceof Menu) {
                                        toolActionButtonsModel.insert(index, toolButton.createObject(null, {
                                            submenu: object
                                        }))
                                    } else {
                                        toolActionButtonsModel.insert(index, toolButton.createObject(null, {
                                            visible: false
                                        }))
                                    }
                                }
                                onObjectRemoved: (index, object) => {
                                    const o = toolActionButtonsModel.get(o)
                                    toolActionButtonsModel.remove(index)
                                    o.destroy()
                                }
                            }
                        }
                    }
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
                name: qsTr("New project"),
                path: "",
                lastModifiedText: "",
                thumbnail: "",
                icon: "",
            })
            ColumnLayout {
                id: recentFilesLayout
                spacing: 16
                anchors.fill: parent
                Item {
                    Layout.fillWidth: true
                    height: titleBarArea.height - recentFilesLayout.spacing - recentFiles.topPadding
                    visible: windowAgent.framelessSetup
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    TextField {
                        id: searchTextField
                        placeholderText: qsTr("Search")
                        Accessible.name: qsTr("Search")
                        Layout.fillWidth: true
                        ThemedItem.icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Search16Filled.svg"
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
                        anchors.left: parent.left
                        anchors.right: parent.right
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        visible: searchTextField.length !== 0 && (fileGridLayout.visibleChildren.length === 1 || fileListLayout.visibleChildren.length === 1)
                    }
                    Label {
                        text: qsTr("No recovery file\nIf %1 crashes, automatic recovery files will be displayed here.").replace("%1", Application.name)
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        visible: recoveryFilesButton.checked && searchTextField.length === 0 && (window.recoveryFilesModel?.count ?? 0) === 0
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
                    visible: window.recentFilesListView && !recoveryFilesButton.checked
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
                            model: window.recentFilesModel
                            ListItemButton {
                                Layout.fillWidth: true
                                visible: modelData.name.toLowerCase().indexOf(searchTextField.text.toLowerCase()) !== -1
                            }
                        }
                    }
                }
                ScrollView {
                    id: recoveryFileListScrollView
                    visible: recoveryFilesButton.checked
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout {
                        id: recoveryFileListLayout
                        spacing: 4
                        implicitWidth: recoveryFileListScrollView.width
                        width: recoveryFileListScrollView.width
                        Repeater {
                            model: window.recoveryFilesModel
                            ListItemButton {
                                Layout.fillWidth: true
                                visible: modelData.name.toLowerCase().indexOf(searchTextField.text.toLowerCase()) !== -1
                                recovery: true
                            }
                        }
                    }
                }
            }
        }
    }
}