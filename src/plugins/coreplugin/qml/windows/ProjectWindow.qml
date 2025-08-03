import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

import SVSCraft
import SVSCraft.UIComponents

import QActionKit

import DiffScope.UIShell
import DiffScope.CorePlugin

ProjectWindow {
    id: projectWindow

    required property ProjectWindowData windowData
    readonly property DockingPane activeDockingPaneObject: {
        if (windowData.activeUndockedPane)
            return windowData.activeUndockedPane
        return dockingPanes[windowData.activePanel] ?? null
    }
    readonly property list<QtObject> dockingPanes: [
        null,
        leftDockingView.firstItem,
        leftDockingView.lastItem,
        rightDockingView.firstItem,
        rightDockingView.lastItem,
        topDockingView.firstItem,
        topDockingView.lastItem,
        bottomDockingView.firstItem,
        bottomDockingView.lastItem
    ]
    readonly property list<QtObject> floatingPanes: [
        ...leftDockingView.undockedItems,
        ...rightDockingView.undockedItems,
        ...topDockingView.undockedItems,
        ...bottomDockingView.undockedItems
    ]
    readonly property list<QtObject> allPanes: [
        ...leftDockingView.contentData,
        ...rightDockingView.contentData,
        ...topDockingView.contentData,
        ...bottomDockingView.contentData
    ].filter(o => o instanceof DockingPane)

    Component.onCompleted: () => {
        windowData.loadVisibility(this)
    }

    menusModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "core.mainMenu"
            context: projectWindow.windowData.actionContext
            onObjectAdded: (index, object) => {
                projectWindow.menusModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                projectWindow.menusModel.remove(index)
            }
        }
    }
    toolButtonsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "core.mainToolBar"
            context: projectWindow.windowData.actionContext
            separatorComponent: ToolBarContainerSeparator {
            }
            stretchComponent: ToolBarContainerStretch {
            }
            onObjectAdded: (index, object) => {
                projectWindow.toolButtonsModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                projectWindow.toolButtonsModel.remove(index)
            }
        }
    }

    property bool savingCurrentLayout: false

    onClosing: () => {
        saveCurrentLayout()
        cleanupPanes()
    }

    function initializeDockingViews() {
        const conv = v => {
            if ((v.geometry?.width ?? 0) !== 0) {
                v.object.Docking.windowGeometryHint = v.geometry
            }
            return v.object
        }
        const f = (dockingView, edge) => {
            let o = windowData.createDockingViewContents(edge)
            dockingView.contentData = o.objects.map(conv)
            dockingView.preferredPanelSize = o.preferredPanelSize
            dockingView.splitterRatio = o.splitterRatio
            for (let i of o.visibleIndices) {
                dockingView.showPane(i)
            }
        }
        f(leftDockingView, Qt.LeftEdge)
        f(rightDockingView, Qt.RightEdge)

        let topData = windowData.createDockingViewContents(Qt.TopEdge)
        let bottomData = windowData.createDockingViewContents(Qt.BottomEdge)
        topDockingView.contentData = topData.objects.map(conv)
        topDockingView.splitterRatio = topData.splitterRatio
        bottomDockingView.contentData = bottomData.objects.map(conv)
        bottomDockingView.splitterRatio = bottomData.splitterRatio
        topDockingViewHeightRatio = (topDockingView.barSize + topData.preferredPanelSize) / (topDockingView.barSize + topData.preferredPanelSize + bottomDockingView.barSize + bottomData.preferredPanelSize)
        for (let i of topData.visibleIndices) {
            topDockingView.showPane(i)
        }
        for (let i of bottomData.visibleIndices) {
            bottomDockingView.showPane(i)
        }
    }

    function saveCurrentLayout() {
        savingCurrentLayout = true
        let viewSpecMap = []
        const f = (dockingView, isLeftOrRight, firstKey, lastKey) => {
            viewSpecMap[firstKey] = {
                panels: [...dockingView.contentData].slice(0, dockingView.stretchIndex).map(o => ({
                    id: o.ActionInstantiator.id,
                    dock: o.dock,
                    opened: o.Docking.window?.visible ?? false,
                    geometry: Qt.rect(o.Docking.window?.x ?? 0, o.Docking.window?.y ?? 0, o.Docking.window?.width ?? 0, o.Docking.window?.height ?? 0)
                })),
                width: isLeftOrRight ? dockingView.panelSize : dockingView.splitterRatio,
                height: isLeftOrRight ? dockingView.splitterRatio : dockingView.panelSize,
                visibleIndex: dockingView.firstIndex,
            }
            viewSpecMap[lastKey] = {
                panels: [...dockingView.contentData].slice(dockingView.stretchIndex + 1).map(o => ({
                    id: o.ActionInstantiator.id,
                    dock: o.dock,
                    opened: o.Docking.window?.visible ?? false,
                    geometry: Qt.rect(o.Docking.window?.x ?? 0, o.Docking.window?.y ?? 0, o.Docking.window?.width ?? 0, o.Docking.window?.height ?? 0)
                })),
                width: isLeftOrRight ? dockingView.panelSize : 1 - dockingView.splitterRatio,
                height: isLeftOrRight ? 1 - dockingView.splitterRatio : dockingView.panelSize,
                visibleIndex: dockingView.lastIndex - (dockingView.stretchIndex + 1),
            }
        }
        f(leftDockingView, true, ProjectWindowData.LeftTop, ProjectWindowData.LeftBottom)
        f(rightDockingView, true, ProjectWindowData.RightTop, ProjectWindowData.RightBottom)
        f(topDockingView, false, ProjectWindowData.TopLeft, ProjectWindowData.TopRight)
        f(bottomDockingView, false, ProjectWindowData.BottomLeft, ProjectWindowData.BottomRight)
        windowData.workspaceManager.currentLayout.setViewSpecFromJavaScript(viewSpecMap)
        savingCurrentLayout = false
    }

    function cleanupPanes() {
        let objects = []
        for (let dockingView of [projectWindow.leftDockingView, projectWindow.rightDockingView, projectWindow.topDockingView, projectWindow.bottomDockingView]) {
            for (let o of dockingView.contentData) {
                objects.push(o)
            }
            dockingView.clearContents()
        }
        for (let o of objects) {
            o.Docking.window?.destroy()
            o.destroy()
        }

    }

    function promptSaveLayout() {
        saveLayoutPopup.open()
    }

    Connections {
        target: projectWindow.windowData
        function onInitialized() {
            Qt.callLater(() => {
                projectWindow.initializeDockingViews()
            })
        }
    }

    Connections {
        target: projectWindow.windowData.workspaceManager
        enabled: !projectWindow.savingCurrentLayout
        function onCurrentLayoutChanged() {
            // TODO: confirm that all objects are destroyed by GC when they are deref-ed
            cleanupPanes()
            projectWindow.initializeDockingViews()
        }
    }

    Component {
        id: dockingStretchComponent
        DockingStretch {
        }
    }

    Popup {
        id: saveLayoutPopup
        anchors.centerIn: parent
        padding: 16
        popupType: Popup.Item
        width: 400
        onAboutToShow: layoutNameTextField.text = ""
        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            Label {
                text: qsTr("Custom layout name")
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            TextField {
                id: layoutNameTextField
                Layout.fillWidth: true
                Keys.onReturnPressed: saveLayoutOkButton.animateClick()
            }
            Label {
                text: qsTr("Custom presets with the same name will be overwritten.")
                color: Theme.warningColor
                Layout.fillWidth: true
                opacity: saveLayoutPopup.visible && [...projectWindow.windowData.workspaceManager.customLayouts].some(o => o.name === layoutNameTextField.text) ? 1 : 0
                wrapMode: Text.Wrap
            }
            Button {
                id: saveLayoutOkButton
                ThemedItem.controlType: SVS.CT_Accent
                text: qsTr("OK")
                enabled: layoutNameTextField.text.length !== 0
                Layout.fillWidth: true
                onClicked: () => {
                    projectWindow.saveCurrentLayout()
                    projectWindow.windowData.saveCustomLayoutFromJavaScript(layoutNameTextField.text)
                    saveLayoutPopup.close()
                }
            }
        }
    }

    component DockingViewBindingHelper: QtObject {
        id: helper
        required property DockingView dockingView
        required property int firstKey
        required property int lastKey
        readonly property Binding binding: Binding {
            helper.dockingView.firstActive: !projectWindow.windowData.activeUndockedPane && projectWindow.windowData.activePanel === helper.firstKey
            helper.dockingView.lastActive: !projectWindow.windowData.activeUndockedPane && projectWindow.windowData.activePanel === helper.lastKey
            helper.dockingView.activeUndockedPane: projectWindow.windowData.activeUndockedPane
        }
        readonly property Connections connections: Connections {
            target: helper.dockingView
            function onFirstActivated() {
                projectWindow.windowData.activeUndockedPane = null
                projectWindow.windowData.activePanel = helper.firstKey
            }
            function onLastActivated() {
                projectWindow.windowData.activeUndockedPane = null
                projectWindow.windowData.activePanel = helper.lastKey
            }
            function onUndockedActivated(pane) {
                projectWindow.windowData.activeUndockedPane = pane
            }
            function onUndockedDeactivated(pane) {
                if (projectWindow.windowData.activeUndockedPane === pane)
                    projectWindow.windowData.activeUndockedPane = null
            }
        }
    }

    DockingViewBindingHelper { dockingView: projectWindow.leftDockingView; firstKey: ProjectWindowData.LeftTop; lastKey: ProjectWindowData.LeftBottom }
    DockingViewBindingHelper { dockingView: projectWindow.rightDockingView; firstKey: ProjectWindowData.RightTop; lastKey: ProjectWindowData.RightBottom }
    DockingViewBindingHelper { dockingView: projectWindow.topDockingView; firstKey: ProjectWindowData.TopLeft; lastKey: ProjectWindowData.TopRight }
    DockingViewBindingHelper { dockingView: projectWindow.bottomDockingView; firstKey: ProjectWindowData.BottomLeft; lastKey: ProjectWindowData.BottomRight }

    onActiveDockingPaneObjectChanged: windowData.activeDockingPane = activeDockingPaneObject
    onDockingPanesChanged: windowData.dockingPanes = [...dockingPanes]
    onFloatingPanesChanged: () => windowData.floatingPanes = [...floatingPanes]
}