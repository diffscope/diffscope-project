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
    title: `${documentName} - ${Application.name}`

    property bool frameless: true
    property url icon: ""
    property string documentName: ""
    property ObjectModel menusModel: null
    property ObjectModel toolButtonsModel: null
    property ObjectModel statusButtonsModel: null
    property ObjectModel bubbleNotificationsModel: null
    property double topDockingViewHeightRatio: 0.3
    property bool useSeparatedMenu: false

    readonly property MenuBar menuBar: menuBar
    readonly property Item toolBar: toolBar
    readonly property Item statusBar: statusBar
    readonly property DockingView leftDockingView: leftDock
    readonly property DockingView rightDockingView: rightDock
    readonly property DockingView topDockingView: topDock
    readonly property DockingView bottomDockingView: bottomDock
    readonly property CommandPalette commandPalette: commandPalettePopup

    function setupFrameless() {
        if (frameless && !windowAgent.framelessSetup) {
            windowAgent.setup(window)
            windowAgent.framelessSetup = true
            windowAgent.setTitleBar(titleBarArea)
            windowAgent.setSystemButton(WindowAgent.Minimize, minimizeSystemButton)
            windowAgent.setSystemButton(WindowAgent.Maximize, maximizeSystemButton)
            windowAgent.setSystemButton(WindowAgent.Close, closeSystemButton)
            windowAgent.setSystemButton(WindowAgent.WindowIcon, iconArea)
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
        y: titleBar.height + verticalOffset
        emptyText: qsTr("Empty")
        recentlyUsedText: qsTr("recently used")
    }
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundQuaternaryColor
    }
    MenuBar {
        id: menuBar
        parent: window.useSeparatedMenu ? separatedMenuParent : titleBarMenuParent
        padding: 0
        leftPadding: 4
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
        spacing: 1
        anchors.fill: parent
        Rectangle {
            id: titleBar
            Accessible.role: Accessible.TitleBar
            Layout.fillWidth: true
            height: Qt.platform.os !== "osx" && Qt.platform.os !== "macos" ? 32 : 28
            color: Theme.backgroundPrimaryColor
            RowLayout {
                anchors.fill: parent
                spacing: 0
                Item {
                    id: iconArea
                    visible: windowAgent.framelessSetup && Qt.platform.os !== "osx" && Qt.platform.os !== "macos"
                    Layout.fillHeight: true
                    width: 40
                    Image {
                        anchors.centerIn: parent
                        source: window.icon
                        width: 16
                        height: 16
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
                    visible: windowAgent.framelessSetup
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
            }
            RowLayout {
                id: titleTextGroup
                visible: windowAgent.framelessSetup
                height: parent.height
                spacing: 4
                Image {
                    visible: Qt.platform.os === "osx" || Qt.platform.os === "macos"
                    Layout.alignment: Qt.AlignVCenter
                    source: window.icon
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                }
                Text {
                    color: Window.active ? Theme.foregroundPrimaryColor : Theme.foregroundSecondaryColor
                    Layout.alignment: Qt.AlignVCenter
                    font: Theme.font
                    text: Window.window.title
                }
                x: {
                    if (LayoutMirroring.enabled) {

                    } else {
                        const middleX = (parent.width - width) / 2
                        return Math.max(middleX, titleBarMenuParent.mapToItem(titleBar, 0, 0).x + titleBarMenuParent.width)
                    }
                }
            }
        }
        Rectangle {
            id: separatedMenuParent
            Layout.fillWidth: true
            color: Theme.backgroundPrimaryColor
            visible: window.useSeparatedMenu && menuBar.height !== 0
            height: 24
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
            ToolBarContainer {
                id: toolBarContainer
                spacing: 4
                anchors.fill: parent
                Instantiator {
                    model: window.toolButtonsModel
                    onObjectAdded: (index, object) => {
                        if (object instanceof Item) {
                            toolBarContainer.insertItem(index, object)
                        } else if (object instanceof Action) {
                            toolBarContainer.insertAction(index, object)
                        } else if (object instanceof Menu) {
                            toolBarContainer.insertMenu(index, object)
                        } else {
                            toolBarContainer.insertItem(index, dummyItem.createObject(this))
                        }
                    }
                    onObjectRemoved: (index, object) => {
                        if (object instanceof Item) {
                            toolBarContainer.removeItem(object)
                        } else if (object instanceof Action) {
                            toolBarContainer.removeAction(object)
                        } else if (object instanceof Menu) {
                            toolBarContainer.removeMenu(object)
                        } else {
                            toolBarContainer.removeItem(toolBar.itemAt(index))
                        }
                    }
                }
            }
        }
        Item {
            id: mainPane
            readonly property double minimumPanelSize: 64
            Layout.fillWidth: true
            Layout.fillHeight: true
            SplitView {
                anchors.fill: parent
                ThemedItem.splitHandleEnabled: rightDock.panelOpened
                Accessible.ignored: true
                SplitView {
                    SplitView.minimumWidth: leftDock.SplitView.minimumWidth + mainPane.minimumPanelSize
                    SplitView.fillWidth: true
                    ThemedItem.splitHandleEnabled: leftDock.panelOpened
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
                                color: Theme.splitterColor
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
                                color: Theme.splitterColor
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
                duration: Theme.visualEffectAnimationDuration
            }
        }
        move: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutCubic
                duration: Theme.visualEffectAnimationDuration
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