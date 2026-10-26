// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

Window {
    id: window
    property alias workspaceContent: workspaceArea.data
    width: 1600
    height: 960
    minimumWidth: 360
    title: documentName
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    property bool frameless: true
    property url icon: ""
    property string documentName: ""
    property bool windowModified: false
    property ObjectModel menusModel: null
    property ObjectModel menusModelForMainMenu: null
    property ObjectModel leftToolButtonsModel: null
    property ObjectModel middleToolButtonsModel: null
    property ObjectModel rightToolButtonsModel: null
    property ObjectModel statusButtonsModel: null
    property ObjectModel bubbleNotificationsModel: null
    property bool useSeparatedMenu: false
    property bool useLeftSystemButton: false

    enum TitleBarStype {
        Style_Windows,
        Style_MacOS,
        Style_Simple
    }
    property int titleBarStyle: ProjectWindow.Style_MacOS

    readonly property bool isMacOS: Qt.platform.os === "osx" || Qt.platform.os === "macos"
    readonly property MenuBar menuBar: menuBar
    readonly property Item toolBar: toolBar
    readonly property Item statusBar: statusBar
    readonly property CommandPalette commandPalette: commandPalettePopup
    readonly property InputPalette inputPalette: inputPalettePopup
    readonly property double popupTopMarginHint: workspaceArea.y + 2

    property bool notificationEnablesAnimation: false

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

    function setupFrameless() {
        if (frameless && !windowAgent.framelessSetup) {
            windowAgent.setup(window)
            windowAgent.framelessSetup = true
            windowAgent.setTitleBar(titleBarArea)
            windowAgent.setSystemButton(WindowAgent.Minimize, minimizeSystemButton)
            windowAgent.setSystemButton(WindowAgent.Maximize, maximizeSystemButton)
            windowAgent.setSystemButton(WindowAgent.Close, closeSystemButton)
            windowAgent.setSystemButton(WindowAgent.WindowIcon, windowIcon)
            windowAgent.setHitTestVisible(Overlay.overlay)
            windowAgent.setHitTestVisible(leftToolBarContainer)
            windowAgent.setHitTestVisible(rightToolBarContainer)
            windowAgent.setHitTestVisible(middleToolBarContainer)
            windowAgent.setHitTestVisible(menuBar)
            windowAgent.setHitTestVisible(mainMenuButton)
        }
    }

    Component.onCompleted: () => {
        setupFrameless()
    }

    onFramelessChanged: () => {
        setupFrameless()
    }

    WindowAgent {
        id: windowAgent
        property bool framelessSetup: false
    }

    PlatformWindowHelper {
        id: platformWindowHelper
        window: window
        windowModified: window.windowModified
    }

    Component {
        id: dummyItem
        Item {
            visible: false
        }
    }

    CommandPalette {
        id: commandPalettePopup
        property double horizontalOffset: 0
        property double verticalOffset: 0
        x: (window.width - implicitWidth) / 2 + horizontalOffset
        y: popupTopMarginHint + verticalOffset
        emptyText: qsTr("Empty")
    }
    InputPalette {
        id: inputPalettePopup
        property double horizontalOffset: 0
        property double verticalOffset: 0
        x: (window.width - implicitWidth) / 2 + horizontalOffset
        y: popupTopMarginHint + verticalOffset
    }
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundQuaternaryColor
    }
    MenuBar {
        id: menuBar
        property bool alwaysVisible: true
        parent: window.isMacOS ? window.contentItem : !windowAgent.framelessSetup || window.useSeparatedMenu ? separatedMenuParent : window.titleBarStyle === ProjectWindow.Style_MacOS ? window.contentItem : titleBarMenuParent
        padding: 0
        leftPadding: 4
        topPadding: !windowAgent.framelessSetup || window.useSeparatedMenu ? 0 : (titleBar.height - 24) / 2
        bottomPadding: !windowAgent.framelessSetup || window.useSeparatedMenu ? 0 : (titleBar.height - 24) / 2
        visible: window.isMacOS || alwaysVisible && (!windowAgent.framelessSetup || window.titleBarStyle !== ProjectWindow.Style_MacOS && (window.titleBarStyle !== ProjectWindow.Style_Simple || window.useSeparatedMenu))
        background: Item {
        }
        Instantiator {
            model: window.menusModel
            onObjectAdded: (index, object) => {
                if (object instanceof Menu) {
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
    ColumnLayout {
        spacing: 0
        anchors.fill: parent
        component PaneSeparator: Rectangle {
            color: Theme.paneSeparatorColor
            implicitHeight: 1
            Layout.fillWidth: true
        }
        Rectangle { // TODO macOS fullscreen transition
            id: titleBar
            Accessible.role: Accessible.TitleBar
            Layout.fillWidth: true
            height: {
                switch (window.titleBarStyle) {
                    case ProjectWindow.Style_Windows:
                        return 32
                    case ProjectWindow.Style_MacOS:
                        return 28
                    case ProjectWindow.Style_Simple:
                        return 32
                }
            }
            color: Theme.backgroundPrimaryColor
            visible: windowAgent.framelessSetup && (!window.isMacOS || window.visibility !== Window.FullScreen)
            Item {
                id: titleBarArea
                anchors.left: parent.left
                anchors.right: parent.right
                y: titleBarSpacer.y
                height: titleBarSpacer.height
            }
            RowLayout {
                anchors.fill: parent
                spacing: 0
                Item {
                    id: iconArea
                    visible: window.titleBarStyle === ProjectWindow.Style_Windows
                    Layout.fillHeight: true
                    width: 40
                    Image {
                        id: windowIcon
                        anchors.centerIn: parent
                        source: window.icon
                        width: 16
                        height: 16
                        sourceSize.width: 16
                        sourceSize.height: 16
                        mipmap: true
                    }
                }
                RowLayout {
                    id: titleBarMenuParent
                    visible: window.titleBarStyle === ProjectWindow.Style_Windows && !window.useSeparatedMenu && menuBar.height !== 0
                }
                Item {
                    id: titleBarSpacer
                    Layout.fillWidth: true
                    Layout.fillHeight: window.titleBarStyle !== ProjectWindow.Style_MacOS
                    Layout.preferredHeight: window.titleBarStyle === ProjectWindow.Style_MacOS ? titleBar.height + (toolBar.visible ? toolBar.height : 0) : 0
                }
            }
            RowLayout {
                id: titleTextGroup
                readonly property bool menuBarMergedInTitleBar: !window.isMacOS && !window.useSeparatedMenu && menuBar.visible
                visible: windowAgent.framelessSetup
                height: parent.height
                spacing: 4
                Image {
                    visible: window.titleBarStyle !== ProjectWindow.Style_Windows
                    Layout.alignment: Qt.AlignVCenter
                    source: window.icon
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    sourceSize.width: 16
                    sourceSize.height: 16
                }
                Text {
                    readonly property color _baseColor: !titleTextGroup.menuBarMergedInTitleBar || window.titleBarStyle !== ProjectWindow.Style_Windows ? Theme.foregroundPrimaryColor : Theme.foregroundSecondaryColor
                    color: Window.active ? _baseColor : Theme.foregroundDisabledColorChange.apply(_baseColor)
                    Layout.alignment: Qt.AlignVCenter
                    font.family: Theme.font.family
                    font.weight: window.titleBarStyle === ProjectWindow.Style_MacOS ? Font.ExtraBold : Font.Normal
                    text: platformWindowHelper.windowTitle
                }
                x: {
                    if (window.titleBarStyle !== ProjectWindow.Style_Windows) {
                        return (parent.width - width) / 2
                    } else if (LayoutMirroring.enabled) {
                        return (!menuBarMergedInTitleBar ? iconArea.x : titleBarMenuParent.x - 24) - width
                    } else {
                        return (!menuBarMergedInTitleBar ? iconArea.x + iconArea.width : titleBarMenuParent.x + titleBarMenuParent.width + 24)
                    }
                }
            }
            Rectangle {
                visible: window.titleBarStyle === ProjectWindow.Style_MacOS && toolBar.visible
                width: parent.width
                height: 1
                anchors.top: parent.bottom
                color: parent.color
            }
            RowLayout {
                x: window.useLeftSystemButton && window.titleBarStyle !== ProjectWindow.Style_Windows ? 0 : parent.width - width
                layoutDirection: window.useLeftSystemButton && window.titleBarStyle !== ProjectWindow.Style_Windows ? Qt.RightToLeft : Qt.LeftToRight
                height: parent.height
                visible: !window.isMacOS
                spacing: 0
                ToolButton {
                    id: mainMenuButton
                    icon.source: "image://fluent-system-icons/more_horizontal"
                    flat: false
                    visible: window.titleBarStyle === ProjectWindow.Style_Simple && !window.useSeparatedMenu
                    text: qsTr("&Main Menu")
                    display: AbstractButton.IconOnly
                    action: MenuAction {
                        menu: Menu {
                            id: mainMenu
                            Instantiator {
                                model: window.menusModelForMainMenu
                                onObjectAdded: (index, object) => {
                                    if (object instanceof Item) {
                                        mainMenu.insertItem(index, object)
                                    } else if (object instanceof Action) {
                                        mainMenu.insertAction(index, object)
                                    } else if (object instanceof Menu) {
                                        mainMenu.insertMenu(index, object)
                                    } else {
                                        mainMenu.insertItem(index, dummyItem.createObject(this))
                                    }
                                }
                                onObjectRemoved: (index, object) => {
                                    if (object instanceof Item) {
                                        mainMenu.removeItem(object)
                                    } else if (object instanceof Action) {
                                        mainMenu.removeAction(object)
                                    } else if (object instanceof Menu) {
                                        mainMenu.removeMenu(object)
                                    } else {
                                        mainMenu.removeItem(target.itemAt(index))
                                    }
                                }
                            }
                        }
                    }
                }
                Item {
                    implicitWidth: 8
                }
                SystemButton {
                    id: minimizeSystemButton
                    type: SystemButton.Minimize
                    useWindowsStyle: window.titleBarStyle === ProjectWindow.Style_Windows
                    Layout.fillHeight: true
                }
                SystemButton {
                    id: maximizeSystemButton
                    type: SystemButton.MaximizeRestore
                    useWindowsStyle: window.titleBarStyle === ProjectWindow.Style_Windows
                    Layout.fillHeight: true
                }
                SystemButton {
                    id: closeSystemButton
                    type: SystemButton.Close
                    useWindowsStyle: window.titleBarStyle === ProjectWindow.Style_Windows
                    Layout.fillHeight: true
                }
            }
        }
        PaneSeparator {
            visible: !window.isMacOS && (!windowAgent.framelessSetup || window.titleBarStyle !== ProjectWindow.Style_MacOS && ((separatedMenuParent.visible && menuBar.visible) || toolBar.visible))
        }
        Rectangle {
            id: separatedMenuParent
            Layout.fillWidth: true
            color: Theme.backgroundPrimaryColor
            visible: !window.isMacOS && (!windowAgent.framelessSetup || window.titleBarStyle !== ProjectWindow.Style_MacOS && window.useSeparatedMenu && menuBar.height !== 0)
            Layout.preferredHeight: menuBar.visible ? 24 : 0
            // FIXME remove spacing when visual invisible
        }
        PaneSeparator {
            visible: separatedMenuParent.visible && menuBar.visible && toolBar.visible
        }
        ToolBar {
            id: toolBar
            Layout.fillWidth: true
            component ToolBarContainerInstantiator: Instantiator {
                required property ToolBarContainer target
                onObjectAdded: (index, object) => {
                    if (object instanceof Item) {
                        target.insertItem(index, object)
                    } else if (object instanceof Action) {
                        target.insertAction(index, object)
                    } else if (object instanceof Menu) {
                        target.insertMenu(index, object)
                    } else {
                        target.insertItem(index, dummyItem.createObject(this))
                    }
                }
                onObjectRemoved: (index, object) => {
                    if (object instanceof Item) {
                        target.removeItem(object)
                    } else if (object instanceof Action) {
                        target.removeAction(object)
                    } else if (object instanceof Menu) {
                        target.removeMenu(object)
                    } else {
                        target.removeItem(target.itemAt(index))
                    }
                }
            }
            contentHeight: Math.max(24, middleToolBarContainer.height, leftToolBarContainer.height, rightToolBarContainer.height)
            ToolBarContainer {
                id: middleToolBarContainer
                spacing: 4
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                ToolBarContainerInstantiator {
                    model: window.middleToolButtonsModel
                    target: middleToolBarContainer
                }
            }
            ToolBarContainer {
                id: leftToolBarContainer
                spacing: 4
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                ToolBarContainerInstantiator {
                    model: window.leftToolButtonsModel
                    target: leftToolBarContainer
                }
            }
            ToolBarContainer {
                id: rightToolBarContainer
                spacing: 4
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                ToolBarContainerInstantiator {
                    model: window.rightToolButtonsModel
                    target: rightToolBarContainer
                }
            }
        }
        PaneSeparator {

        }
        Rectangle {
            id: workspaceArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.backgroundPrimaryColor
        }
        PaneSeparator {
            visible: statusBar.visible
        }
        ToolBar {
            id: statusBar
            Accessible.role: Accessible.StatusBar
            Layout.fillWidth: true
            implicitHeight: 24
            topPadding: 0
            bottomPadding: 0
            leftPadding: 8
            rightPadding: 8
            RowLayout {
                spacing: 4
                anchors.fill: parent
                Repeater {
                    model: window.statusButtonsModel
                }
            }
        }
    }
    BubbleNotificationLayer {
        anchors.fill: parent
        model: window.bubbleNotificationsModel
        enablesAnimation: window.notificationEnablesAnimation
        topMargin: 104
        bottomMargin: 64
        leftMargin: 40
        rightMargin: 40
    }
}
