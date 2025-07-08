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
    width: 1024
    height: 800
    background: Rectangle {
        color: Theme.backgroundQuaternaryColor
    }
    title: `${documentName} - ${Application.name}`

    property bool frameless: true
    property url icon: ""
    property string documentName: ""
    property ObjectModel menusModel: null
    property ObjectModel toolButtonsModel: null
    property ObjectModel statusButtonsModel: null

    readonly property DockingView leftDockingView: leftDock
    readonly property DockingView rightDockingView: rightDock
    readonly property DockingView topDockingView: topDock
    readonly property DockingView bottomDockingView: bottomDock

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
                MenuBar {
                    id: menuBar
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
                    color: Theme.foregroundPrimaryColor
                    Layout.alignment: Qt.AlignVCenter
                    text: Window.window.title
                }
                x: {
                    if (LayoutMirroring.enabled) {

                    } else {
                        const middleX = (parent.width - width) / 2
                        return Math.max(middleX, menuBar.mapToItem(titleBar, 0, 0).x + menuBar.width)
                    }
                }
            }
        }
        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                spacing: 4
                anchors.fill: parent
                Repeater {
                    model: window.toolButtonsModel
                }
            }
        }
        Item {
            id: mainPane
            readonly property double minimumPanelSize: 100
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
                            SplitView.preferredHeight: (middleSplitView.height - 1) / 2
                            SplitView.maximumHeight: Math.max(SplitView.minimumHeight, !topDock.panelOpened ? topDock.barSize : Infinity)
                            DockingView {
                                id: topDock
                                Accessible.name: qsTr("Top Docking View")
                                width: parent.width
                                anchors.top: parent.top
                                edge: Qt.TopEdge
                                property double preferredPanelSize: 400
                                panelSize: parent.height - barSize
                                onPanelSizeChanged: () => {
                                    if (panelSize > 0)
                                        preferredPanelSize = panelSize
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
                            SplitView.preferredHeight: (middleSplitView.height - 1) / 2
                            SplitView.maximumHeight: Math.max(SplitView.minimumHeight, !bottomDock.panelOpened ? bottomDock.barSize : Infinity)
                            DockingView {
                                id: bottomDock
                                Accessible.name: qsTr("Bottom Docking View")
                                width: parent.width
                                anchors.bottom: parent.bottom
                                edge: Qt.BottomEdge
                                property double preferredPanelSize: 400
                                panelSize: parent.height - barSize
                                onPanelSizeChanged: () => {
                                    if (panelSize > 0)
                                        preferredPanelSize = panelSize
                                }
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

}