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
    property var recentFilesModel: null
    property bool recentFilesIsListView: false
    property var recoveryFilesModel: null
    property var panelsModel: null
    property var navigationActionsModel: null
    property var toolActionsModel: null
    property var menusModel: null
    property int currentNavIndex: 0

    readonly property bool isMacOS: Qt.platform.os === "osx" || Qt.platform.os === "macos"

    readonly property CommandPalette commandPalette: commandPalettePopup
    readonly property InputPalette inputPalette: inputPalettePopup
    readonly property double popupTopMarginHint: (titleBarArea.visible ? titleBarArea.height : 8)

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

    onCurrentNavIndexChanged: () => {
        navItemsModel.get(currentNavIndex)?.click()
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
            parent: window.isMacOS ? window.contentItem : titleBarArea
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
                    Repeater {
                        model: ObjectModel {
                            id: navItemsModel
                            readonly property Component navButtonComponent: NavButton {
                                id: button
                                required property QtObject pane
                                text: pane.title
                                Accessible.role: Accessible.RadioButton
                                checkable: true
                                autoExclusive: true
                                icon: pane.icon
                                Rectangle {
                                    width: button.pane.badgeNumber > 0 ? Math.max(16, numberText.width) : 8
                                    height: button.pane.badgeNumber > 0 ? 16 : 8
                                    radius: button.pane.badgeNumber > 0 ? 8 : 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.right
                                    anchors.rightMargin: 6
                                    color: Theme.controlColor(button.pane.badgeType)
                                    visible: button.pane.badgeNumber !== 0
                                    Label {
                                        id: numberText
                                        padding: 2
                                        anchors.centerIn: parent
                                        horizontalAlignment: Text.AlignHCenter
                                        font.pixelSize: 10
                                        text: button.pane.badgeNumber.toLocaleString()
                                    }
                                }
                                onClicked: () => {
                                    if (checked) {
                                        recentFilesStack.pane = pane
                                    }
                                }
                            }
                            readonly property Instantiator instantiator: Instantiator {
                                model: window.panelsModel
                                onObjectAdded: (index, object) => {
                                    navItemsModel.insert(index, navItemsModel.navButtonComponent.createObject(navLayout, {pane: object}))
                                }
                                onObjectRemoved: (index, object) => {
                                    navItemsModel.remove(index)
                                }
                            }
                            onCountChanged: get(window.currentNavIndex).click()
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
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.backgroundQuaternaryColor
            ColumnLayout {
                id: recentFilesLayout
                spacing: 16
                anchors.fill: parent
                Item {
                    Layout.fillWidth: true
                    implicitHeight: titleBarArea.visible ? titleBarArea.height - recentFilesLayout.spacing : 0
                }
                StackLayout {
                    id: recentFilesStack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    property Item pane: null
                    data: [pane]
                }
            }
        }
    }
}