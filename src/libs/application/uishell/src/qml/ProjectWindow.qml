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
    width: 1280
    height: 800
    minimumWidth: 360
    title: `${documentName} - ${Application.displayName}`
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    property bool frameless: true
    property url icon: ""
    property string documentName: ""
    property ObjectModel menusModel: null
    property ObjectModel leftToolButtonsModel: null
    property ObjectModel middleToolButtonsModel: null
    property ObjectModel rightToolButtonsModel: null
    property ObjectModel statusButtonsModel: null
    property ObjectModel bubbleNotificationsModel: null
    property double topDockingViewHeightRatio: 0.5
    property bool useSeparatedMenu: false

    readonly property bool isMacOS: Qt.platform.os === "osx" || Qt.platform.os === "macos"
    readonly property MenuBar menuBar: menuBar
    readonly property Item toolBar: toolBar
    readonly property Item statusBar: statusBar
    readonly property DockingView leftDockingView: leftDock
    readonly property DockingView rightDockingView: rightDock
    readonly property DockingView topDockingView: topDock
    readonly property DockingView bottomDockingView: bottomDock
    readonly property CommandPalette commandPalette: commandPalettePopup
    readonly property InputPalette inputPalette: inputPalettePopup

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
            windowAgent.setSystemButton(WindowAgent.WindowIcon, iconArea)
            windowAgent.setHitTestVisible(Overlay.overlay)
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

    CommandPalette {
        id: commandPalettePopup
        property double horizontalOffset: 0
        property double verticalOffset: 0
        x: (window.width - implicitWidth) / 2 + horizontalOffset
        y: titleBar.height + toolBar.height + 4 + verticalOffset
        emptyText: qsTr("Empty")
    }
    InputPalette {
        id: inputPalettePopup
        property double horizontalOffset: 0
        property double verticalOffset: 0
        x: (window.width - implicitWidth) / 2 + horizontalOffset
        y: titleBar.height + toolBar.height + 4 + verticalOffset
    }
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundQuaternaryColor
    }
    MenuBar {
        id: menuBar
        property bool alwaysVisible: true
        readonly property bool visualVisible: alwaysVisible || activeFocus || menus.some(menu => menu.visible) || children.some(item => item.activeFocus)
        parent: window.isMacOS ? window.contentItem : !windowAgent.framelessSetup || window.useSeparatedMenu ? separatedMenuParent : titleBarMenuParent
        padding: 0
        leftPadding: 4
        topPadding: !windowAgent.framelessSetup || window.useSeparatedMenu ? 0 : (titleBar.height - 24) / 2
        bottomPadding: !windowAgent.framelessSetup || window.useSeparatedMenu ? 0 : (titleBar.height - 24) / 2
        opacity: visualVisible ? 1 : 0
        width: visualVisible ? implicitWidth : 0
        Layout.preferredWidth: visualVisible ? implicitWidth : 0
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
            height: !window.isMacOS ? 32 : 28
            color: Theme.backgroundPrimaryColor
            visible: windowAgent.framelessSetup && (!window.isMacOS || window.visibility !== Window.FullScreen)
            RowLayout {
                anchors.fill: parent
                spacing: 0
                Item {
                    id: iconArea
                    visible: !window.isMacOS
                    Layout.fillHeight: true
                    width: 40
                    Image {
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
                    visible: !window.useSeparatedMenu && menuBar.height !== 0
                }
                Item {
                    id: titleBarArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
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
            }
            RowLayout {
                id: titleTextGroup
                readonly property bool menuBarMergedInTitleBar: !window.isMacOS && !window.useSeparatedMenu && menuBar.visualVisible
                visible: windowAgent.framelessSetup
                height: parent.height
                spacing: 4
                Image {
                    visible: window.isMacOS
                    Layout.alignment: Qt.AlignVCenter
                    source: window.icon
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                }
                Text {
                    readonly property color _baseColor: !titleTextGroup.menuBarMergedInTitleBar ? Theme.foregroundPrimaryColor : Theme.foregroundSecondaryColor
                    color: Window.active ? _baseColor : Theme.foregroundDisabledColorChange.apply(_baseColor)
                    Layout.alignment: Qt.AlignVCenter
                    font: Theme.font
                    text: Window.window.title
                    Component.onCompleted: { font.weight = window.isMacOS ? Font.ExtraBold : Font.Normal }
                }
                x: {
                    if (window.isMacOS) {
                        return (parent.width - width) / 2
                    } else if (LayoutMirroring.enabled) {
                        return (!menuBarMergedInTitleBar ? iconArea.x : titleBarMenuParent.x - 24) - width
                    } else {
                        return (!menuBarMergedInTitleBar ? iconArea.x + iconArea.width : titleBarMenuParent.x + titleBarMenuParent.width + 24)
                    }
                }
            }
            Rectangle {
                visible: window.isMacOS && toolBar.visible
                width: parent.width
                height: 1
                anchors.top: parent.bottom
                color: parent.color
            }
        }
        PaneSeparator {}
        Rectangle {
            id: separatedMenuParent
            Layout.fillWidth: true
            color: Theme.backgroundPrimaryColor
            visible: !window.isMacOS && (!windowAgent.framelessSetup || window.useSeparatedMenu) && menuBar.height !== 0
            implicitHeight: menuBar.visualVisible ? 24 : 0
            // FIXME remove spacing when visual invisible
        }
        PaneSeparator {
            visible: separatedMenuParent.visible && menuBar.visualVisible
        }
        ToolBar {
            id: toolBar
            Layout.fillWidth: true
            Component {
                id: dummyItem
                Item {
                    visible: false
                }
            }
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
        PaneSeparator {}
        Item {
            id: mainPane
            readonly property double minimumPanelSize: 64
            Layout.fillWidth: true
            Layout.fillHeight: true
            SplitView {
                anchors.fill: parent
                ThemedItem.splitHandleEnabled: rightDock.panelOpened
                ThemedItem.dividerStroke: rightDock.panelOpened ? SVS.DS_Splitter : SVS.DS_PaneSeparator
                Accessible.ignored: true
                SplitView {
                    SplitView.minimumWidth: leftDock.SplitView.minimumWidth + mainPane.minimumPanelSize
                    SplitView.fillWidth: true
                    ThemedItem.splitHandleEnabled: leftDock.panelOpened
                    ThemedItem.dividerStroke: leftDock.panelOpened ? SVS.DS_Splitter : SVS.DS_PaneSeparator
                    Accessible.ignored: true
                    DockingView {
                        id: leftDock
                        Accessible.name: qsTr("Left Docking View")
                        property double preferredPanelSize: 400
                        SplitView.minimumWidth: barSize + (panelOpened ? mainPane.minimumPanelSize : 0)
                        SplitView.preferredWidth: barSize + (panelOpened ? preferredPanelSize : 0)
                        SplitView.maximumWidth: !panelOpened ? barSize : Infinity
                        clip: true
                        panelSize: width - barSize
                        onPanelSizeChanged: () => {
                            if (panelSize > 0)
                                preferredPanelSize = panelSize
                        }
                        barBackgroundLevel: SVS.BL_Secondary
                    }
                    SplitView {
                        id: middleSplitView
                        orientation: Qt.Vertical
                        SplitView.fillWidth: true
                        SplitView.fillHeight: true
                        SplitView.minimumWidth: mainPane.minimumPanelSize
                        ThemedItem.splitHandleVisible: topDock.panelOpened || bottomDock.panelOpened
                        ThemedItem.splitHandleEnabled: topDock.panelOpened && bottomDock.panelOpened
                        ThemedItem.dividerStroke: topDock.panelOpened && bottomDock.panelOpened ? SVS.DS_Splitter : SVS.DS_PaneSeparator
                        Accessible.ignored: true
                        Item {
                            SplitView.minimumHeight: !bottomDock.panelOpened ? middleSplitView.height - bottomDock.barSize - 1 : topDock.barSize + (topDock.panelOpened ? mainPane.minimumPanelSize : 0)
                            SplitView.preferredHeight: window.topDockingViewHeightRatio * (middleSplitView.height - 1)
                            SplitView.maximumHeight: Math.max(SplitView.minimumHeight, !topDock.panelOpened ? topDock.barSize : Infinity)
                            DockingView {
                                id: topDock
                                Accessible.name: qsTr("Top Docking View")
                                width: parent.width
                                anchors.top: parent.top
                                edge: Qt.TopEdge
                                readonly property double preferredPanelSize: window.topDockingViewHeightRatio * (middleSplitView.height - 1) - barSize
                                panelSize: parent.height - barSize
                                onPanelSizeChanged: () => {
                                    if (middleSplitView.resizing) {
                                        window.topDockingViewHeightRatio = height / (middleSplitView.height - 1)
                                    }
                                }
                                barBackgroundLevel: SVS.BL_Secondary
                            }
                            Rectangle {
                                width: parent.width
                                height: 1
                                anchors.top: topDock.bottom
                                color: Theme.paneSeparatorColor
                                visible: !topDock.panelOpened
                            }
                        }
                        Item {
                            SplitView.minimumHeight: bottomDock.barSize + (bottomDock.panelOpened ? mainPane.minimumPanelSize : 0)
                            SplitView.preferredHeight: middleSplitView.height - 1 - topDock.height
                            SplitView.maximumHeight: Math.max(SplitView.minimumHeight, !bottomDock.panelOpened ? bottomDock.barSize : Infinity)
                            DockingView {
                                id: bottomDock
                                Accessible.name: qsTr("Bottom Docking View")
                                width: parent.width
                                anchors.bottom: parent.bottom
                                edge: Qt.BottomEdge
                                readonly property double preferredPanelSize: (1 - window.topDockingViewHeightRatio) * (middleSplitView.height - 1)
                                panelSize: parent.height - barSize
                                barBackgroundLevel: SVS.BL_Secondary
                            }
                            Rectangle {
                                width: parent.width
                                height: 1
                                anchors.bottom: bottomDock.top
                                color: Theme.paneSeparatorColor
                                visible: !bottomDock.panelOpened
                            }
                        }
                    }
                }

                DockingView {
                    id: rightDock
                    Accessible.name: qsTr("Right Docking View")
                    SplitView.minimumWidth: barSize + (panelOpened ? mainPane.minimumPanelSize : 0)
                    SplitView.preferredWidth: barSize + (panelOpened ? preferredPanelSize : 0)
                    SplitView.maximumWidth: !panelOpened ? barSize : Infinity
                    clip: true
                    edge: Qt.RightEdge
                    property double preferredPanelSize: 400
                    panelSize: width - barSize
                    onPanelSizeChanged: () => {
                        if (panelSize > 0)
                            preferredPanelSize = panelSize
                    }
                    barBackgroundLevel: SVS.BL_Secondary
                }
            }
        }
        PaneSeparator {}
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
    Flow {
        id: bubbleNotificationProxyItemFlow
        anchors.fill: parent
        anchors.topMargin: 104
        anchors.bottomMargin: 64
        anchors.leftMargin: 40
        anchors.rightMargin: 40
        spacing: 12
        flow: Flow.TopToBottom
        add: Transition {
            NumberAnimation {
                property: "x"
                from: (bubbleNotificationProxyItemFlow.effectiveLayoutDirection === Qt.LeftToRight ? -360 : bubbleNotificationProxyItemFlow.width + 360)
                easing.type: Easing.OutCubic
                duration: window.notificationEnablesAnimation ? Theme.visualEffectAnimationDuration : 0
            }
        }
        move: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutCubic
                duration: window.notificationEnablesAnimation ? Theme.visualEffectAnimationDuration : 0
            }
        }
        Repeater {
            model: ObjectModel {
                id: bubbleNotificationsProxyItemModel
            }
        }
    }
    Item {
        anchors.fill: parent
        anchors.topMargin: 104
        anchors.bottomMargin: 64
        anchors.leftMargin: 40
        anchors.rightMargin: 40
        Repeater {
            model: ObjectModel {
                id: bubbleNotificationsItemModel
            }
        }
    }
    Instantiator {
        model: window.bubbleNotificationsModel
        readonly property Component bubbleNotification: BubbleNotification {
            id: bubbleNotification
            popupLike: true
            property Item proxyItem: Item {
                readonly property BubbleNotification sourceItem: bubbleNotification
                width: sourceItem.width
                height: sourceItem.height
            }
            x: parent ? parent.width - width - proxyItem.x : 0
            y: parent ? parent.height - height - proxyItem.y : 0
        }
        onObjectAdded: (index, object) => {
            const o = bubbleNotification.createObject(null, {handle: object})
            bubbleNotificationsItemModel.insert(index, o)
            bubbleNotificationsProxyItemModel.insert(index, o.proxyItem)
        }
        onObjectRemoved: (index, object) => {
            const o = bubbleNotificationsItemModel.get(index)
            bubbleNotificationsItemModel.remove(index)
            bubbleNotificationsProxyItemModel.remove(index)
            o.destroy()
        }
    }
}