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

Window {
    id: window
    width: 800
    height: 500
    property bool frameless: true
    property url banner: ""
    property alias recoveryFilesVisible: recoveryFilesButton.checked
    property var recentFilesModel: null
    property bool recentFilesIsListView: false
    property var recoveryFilesModel: null
    property var navigationActionsModel: null
    property var toolActionsModel: null
    property var menusModel: null

    readonly property bool isMacOS: Qt.platform.os === "osx" || Qt.platform.os === "macos"

    readonly property CommandPalette commandPalette: commandPalettePopup
    readonly property InputPalette inputPalette: inputPalettePopup

    readonly property InvisibleCentralWidget invisibleCentralWidget: InvisibleCentralWidget {
        visible: window.visible
        windowHandle.transientParent: window
        geometry: {
            void(window.x)
            void(window.y)
            const p = window.contentItem.mapToGlobal(0, 0)
            return Qt.rect(p.x + window.width / 2, p.y + window.height / 2, 1, 1)
        }
    }

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    signal newFileRequested()
    signal openRecentFileRequested(int index)
    signal openRecoveryFileRequested(int index)
    signal removeRecentFileRequested(int index)
    signal removeRecoveryFileRequested(int index)

    function setupFrameless() {
        if (frameless && !windowAgent.framelessSetup) {
            windowAgent.setup(window)
            windowAgent.framelessSetup = true
            windowAgent.setTitleBar(titleBarArea)
            windowAgent.setSystemButton(WindowAgent.Minimize, minimizeSystemButton)
            windowAgent.setSystemButton(WindowAgent.Maximize, maximizeSystemButton)
            windowAgent.setSystemButton(WindowAgent.Close, closeSystemButton)
            windowAgent.setHitTestVisible(menuBar)
            windowAgent.setHitTestVisible(Overlay.overlay)
        }
    }

    Component.onCompleted: () => {
        setupFrameless()
    }

    onFramelessChanged: () => {
        setupFrameless()
    }

    CommandPalette {
        id: commandPalettePopup
        property double horizontalOffset: 0
        property double verticalOffset: 0
        x: (window.width - implicitWidth) / 2 + horizontalOffset
        y: titleBarArea.height + verticalOffset
        emptyText: qsTr("Empty")
    }
    InputPalette {
        id: inputPalettePopup
        property double horizontalOffset: 0
        property double verticalOffset: 0
        x: (window.width - implicitWidth) / 2 + horizontalOffset
        y: titleBarArea.height + verticalOffset
    }

    RecentFilesProxyModel {
        id: recentFilesProxyModel
        sourceModel: window.recentFilesModel
        filterRole: USDef.RF_NameRole
        filterCaseSensitivity: Qt.CaseInsensitive
        property string _filterRegularExpression: searchTextField.text
        on_FilterRegularExpressionChanged: setFilterRegularExpression(_filterRegularExpression)
    }

    RecentFilesProxyModel {
        id: recoveryFilesProxyModel
        sourceModel: window.recoveryFilesModel
        filterRole: USDef.RF_NameRole
        filterCaseSensitivity: Qt.CaseInsensitive
        property string _filterRegularExpression: searchTextField.text
        on_FilterRegularExpressionChanged: setFilterRegularExpression(_filterRegularExpression)
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
    }
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
                    border.width: 1
                    border.color: Theme.borderColor
                    ColorImage {
                        visible: cell.index === -1
                        width: 80
                        height: 80
                        anchors.centerIn: parent
                        source: "qrc:/qt/qml/DiffScope/UIShell/assets/DocumentAdd48Regular.svg"
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
                window.openRecoveryFileRequested(recoveryFilesProxyModel.mapIndexToSource(cell.index))
            } else {
                window.openRecentFileRequested(recentFilesProxyModel.mapIndexToSource(cell.index))
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
                    source: "qrc:/qt/qml/DiffScope/UIShell/assets/DocumentAdd48Regular.svg"
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
                window.openRecoveryFileRequested(recoveryFilesProxyModel.mapIndexToSource(cell.index))
            } else {
                window.openRecentFileRequested(recentFilesProxyModel.mapIndexToSource(cell.index))
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
                        window.openRecoveryFileRequested(recoveryFilesProxyModel.mapIndexToSource(tapHandler.index))
                    } else {
                        window.openRecentFileRequested(recentFilesProxyModel.mapIndexToSource(tapHandler.index))
                    }
                }
            }
            Action {
                text: qsTr("Reveal in %1").arg(DesktopServices.fileManagerName)
                icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/OpenFolder16Filled.svg"
                enabled: tapHandler.modelData.path.length !== 0
                onTriggered: () => {
                    DesktopServices.reveal(tapHandler.modelData.path)
                }
            }
            Action {
                text: tapHandler.recovery ? qsTr('Remove from "Recovery Files"') : qsTr('Remove from "Recent Files"')
                icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/DocumentDismiss16Filled.svg"
                onTriggered: () => {
                    if (tapHandler.recovery) {
                        window.removeRecoveryFileRequested(recoveryFilesProxyModel.mapIndexToSource(tapHandler.index))
                    } else {
                        window.removeRecentFileRequested(recentFilesProxyModel.mapIndexToSource(tapHandler.index))
                    }
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
        height: !window.isMacOS ? 36 : 28
        visible: windowAgent.framelessSetup && (!window.isMacOS || window.visibility !== Window.FullScreen)
        z: 1
        Accessible.role: Accessible.TitleBar
        Rectangle {
            id: menuBarBackground
            width: parent.width
            height: menuBar.height
            visible: menuBar.height !== 0
            anchors.top: parent.top
            anchors.topMargin: menuBar.anchors.topMargin
            color: Theme.backgroundQuaternaryColor
        }
        MenuBar {
            id: menuBar
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: activeFocus || menus.some(menu => menu.visible) || children.some(item => item.activeFocus) ? 0 : -height
            ThemedItem.backgroundLevel: SVS.BL_Quaternary
            Behavior on anchors.topMargin {
                id: topMarginBehavior
                enabled: false
                NumberAnimation {
                    duration: Theme.visualEffectAnimationDuration
                    easing.type: Easing.OutCubic
                }
            }
            Component.onCompleted: Qt.callLater(() => topMarginBehavior.enabled = true)
            Instantiator {
                model: window.menusModel
                onObjectAdded: (index, object) => {
                    if (object instanceof Menu) {
                        console.log(object)
                        menuBar.insertMenu(index, object)
                    } else {
                        throw new TypeError("Unsupported menu type")
                    }
                }
                onObjectRemoved: (index, object) => {
                    if (object instanceof Menu) {
                        menuBar.removeMenu(object)
                    } else {
                        throw new TypeError("Unsupported menu type")
                    }
                }
            }
        }
        RowLayout {
            anchors.right: parent.right
            visible: !window.isMacOS
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
                    visible: titleBarArea.visible
                    height: titleBarArea.height - nav.topPadding - navLayout.spacing
                }
                Item {
                    Layout.fillWidth: true
                    implicitHeight: banner.implicitHeight * width / banner.implicitWidth
                    Image {
                        id: banner
                        anchors.fill: parent
                        source: window.banner
                        mipmap: true
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
                            visible: recoveryFilesProxyModel.count !== 0
                            Label {
                                id: recoveryFileCountText
                                padding: 2
                                anchors.centerIn: parent
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 10
                                text: recoveryFilesProxyModel.count.toLocaleString()
                            }
                        }
                    }
                }
                MenuSeparator {
                    Layout.fillWidth: true
                }
                component NavContainerInstantiator: Instantiator {
                    required property ToolBarContainer target
                    onObjectAdded: (index, object) => {
                        if (object instanceof Action) {
                            target.insertAction(index, object)
                        } else if (object instanceof Menu) {
                            target.insertMenu(index, object)
                        } else {
                            target.insertItem(index, target.toolButtonComponent.create(this, {visible: false}))
                        }
                    }
                    onObjectRemoved: (index, object) => {
                        if (object instanceof Action) {
                            target.removeAction(object)
                        } else if (object instanceof Menu) {
                            target.removeMenu(object)
                        } else {
                            target.removeItem(target.itemAt(index))
                        }
                    }
                }
                ToolBarContainer {
                    id: navAreaContainer
                    spacing: 6
                    vertical: true
                    Layout.fillWidth: true
                    toolButtonComponent: NavButton {
                    }
                    NavContainerInstantiator {
                        model: window.navigationActionsModel
                        target: navAreaContainer
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                ToolBarContainer {
                    id: navToolBarContainer
                    Layout.leftMargin: 2
                    spacing: 6
                    showMenuAboveButton: true
                    toolButtonComponent: NavToolButton {
                    }
                    NavContainerInstantiator {
                        model: window.toolActionsModel
                        target: navToolBarContainer
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
                    visible: titleBarArea.visible
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
                            checked: !window.recentFilesIsListView
                            onClicked: GlobalHelper.setProperty(window, "recentFilesIsListView", !checked)
                            text: qsTr("Grid view")
                            display: AbstractButton.IconOnly
                        }
                        ToolButton {
                            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/List16Filled.svg"
                            checkable: true
                            autoExclusive: true
                            checked: window.recentFilesIsListView
                            onClicked: GlobalHelper.setProperty(window, "recentFilesIsListView", checked)
                            text: qsTr("List view")
                            display: AbstractButton.IconOnly
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
                        text: qsTr("No recovery file\nIf %1 crashes, automatic recovery files will be displayed here.").arg(Application.displayName)
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        visible: recoveryFilesButton.checked && searchTextField.length === 0 && recoveryFilesProxyModel.count === 0
                    }
                }
                ScrollView {
                    id: fileGridScrollView
                    visible: !window.recentFilesIsListView && !recoveryFilesButton.checked
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
                            model: recentFilesProxyModel
                            CellButton {
                            }
                        }
                    }
                }
                ScrollView {
                    id: fileListScrollView
                    visible: window.recentFilesIsListView && !recoveryFilesButton.checked
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
                            model: recentFilesProxyModel
                            ListItemButton {
                                Layout.fillWidth: true
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
                            model: recoveryFilesProxyModel
                            ListItemButton {
                                Layout.fillWidth: true
                                recovery: true
                            }
                        }
                    }
                }
            }
        }
    }
}