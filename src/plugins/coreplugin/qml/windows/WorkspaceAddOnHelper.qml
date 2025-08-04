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

QtObject {
    id: helper
    required property QtObject addOn
    property IProjectWindow windowHandle: addOn.windowHandle
    property Window window: windowHandle.window

    property DockingPane activeUndockedPane: null
    property int activePanel: -1

    readonly property DockingPane activeDockingPaneObject: {
        if (activeUndockedPane)
            return activeUndockedPane
        return dockingPanes[activePanel] ?? null
    }
    readonly property list<QtObject> dockingPanes: [
        null,
        window.leftDockingView.firstItem,
        window.leftDockingView.lastItem,
        window.rightDockingView.firstItem,
        window.rightDockingView.lastItem,
        window.topDockingView.firstItem,
        window.topDockingView.lastItem,
        window.bottomDockingView.firstItem,
        window.bottomDockingView.lastItem
    ]
    readonly property list<QtObject> floatingPanes: [
        ...window.leftDockingView.undockedItems,
        ...window.rightDockingView.undockedItems,
        ...window.topDockingView.undockedItems,
        ...window.bottomDockingView.undockedItems
    ]
    readonly property list<QtObject> allPanes: [
        ...window.leftDockingView.contentData,
        ...window.rightDockingView.contentData,
        ...window.topDockingView.contentData,
        ...window.bottomDockingView.contentData
    ].filter(o => o instanceof DockingPane)

    property bool savingCurrentLayout: false

    // Keep identical to ProjectWindowWorkspaceLayout
    enum PanelPosition {
        LeftTop,
        LeftBottom,
        RightTop,
        RightBottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    }

    function initializeDockingViews() {
        const conv = v => {
            if ((v.geometry?.width ?? 0) !== 0) {
                v.object.Docking.windowGeometryHint = v.geometry
            }
            return v.object
        }
        const f = (dockingView, edge) => {
            let o = addOn.createDockingViewContents(edge)
            dockingView.contentData = o.objects.map(conv)
            dockingView.preferredPanelSize = o.preferredPanelSize
            dockingView.splitterRatio = o.splitterRatio
            for (let i of o.visibleIndices) {
                dockingView.showPane(i)
            }
        }
        f(window.leftDockingView, Qt.LeftEdge)
        f(window.rightDockingView, Qt.RightEdge)

        let topData = addOn.createDockingViewContents(Qt.TopEdge)
        let bottomData = addOn.createDockingViewContents(Qt.BottomEdge)
        window.topDockingView.contentData = topData.objects.map(conv)
        window.topDockingView.splitterRatio = topData.splitterRatio
        window.bottomDockingView.contentData = bottomData.objects.map(conv)
        window.bottomDockingView.splitterRatio = bottomData.splitterRatio
        window.topDockingViewHeightRatio = (window.topDockingView.barSize + topData.preferredPanelSize) / (window.topDockingView.barSize + topData.preferredPanelSize + window.bottomDockingView.barSize + bottomData.preferredPanelSize)
        for (let i of topData.visibleIndices) {
            window.topDockingView.showPane(i)
        }
        for (let i of bottomData.visibleIndices) {
            window.bottomDockingView.showPane(i)
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
                width: isLeftOrRight ? dockingView.preferredPanelSize : dockingView.splitterRatio,
                height: isLeftOrRight ? dockingView.splitterRatio : dockingView.preferredPanelSize,
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
        f(window.leftDockingView, true, WorkspaceAddOnHelper.LeftTop, WorkspaceAddOnHelper.LeftBottom)
        f(window.rightDockingView, true, WorkspaceAddOnHelper.RightTop, WorkspaceAddOnHelper.RightBottom)
        f(window.topDockingView, false, WorkspaceAddOnHelper.TopLeft, WorkspaceAddOnHelper.TopRight)
        f(window.bottomDockingView, false, WorkspaceAddOnHelper.BottomLeft, WorkspaceAddOnHelper.BottomRight)
        addOn.workspaceManager.currentLayout.setViewSpecFromJavaScript(viewSpecMap)
        savingCurrentLayout = false
    }

    function cleanupPanes() {
        let objects = []
        for (let dockingView of [window.leftDockingView, window.rightDockingView, window.topDockingView, window.bottomDockingView]) {
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

    function promptSaveLayout(layout, originName) {
        saveLayoutPopup.layout = layout
        saveLayoutPopup.originName = originName
        saveLayoutPopup.open()
    }

    function promptDeleteLayout(name) {
        if (window.contentItem.MessageBox.question(qsTr("Delete"), qsTr('Delete layout "%1"?').replace("%1", name)) === SVS.Yes) {
            addOn.removeCustomLayoutFromJavaScript(name)
        }
    }

    readonly property Connections windowHandleConnections: Connections {
        target: helper.windowHandle
        function onInitializationDone() {
            helper.initializeDockingViews()
        }
    }

    readonly property Connections workspaceManagerConnections: Connections {
        target: helper.addOn.workspaceManager
        enabled: !helper.savingCurrentLayout
        function onCurrentLayoutChanged() {
            helper.cleanupPanes()
            helper.initializeDockingViews()
        }
    }

    readonly property Connections windowConnections: Connections {
        target: helper.window
        function onBeforeTerminated() {
            helper.saveCurrentLayout()
            helper.cleanupPanes()
        }
    }

    readonly property Popup saveLayoutPopup: Popup {
        id: saveLayoutPopup
        anchors.centerIn: parent
        padding: 16
        parent: helper.window.contentItem
        width: 400
        property var layout: undefined
        property string originName: ""
        onAboutToShow: layoutNameTextField.text = originName
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
                opacity: saveLayoutPopup.visible && layoutNameTextField.text !== saveLayoutPopup.originName && [...helper.addOn.workspaceManager.customLayouts].some(o => o.name === layoutNameTextField.text) ? 1 : 0
                wrapMode: Text.Wrap
            }
            Button {
                id: saveLayoutOkButton
                ThemedItem.controlType: SVS.CT_Accent
                text: qsTr("OK")
                enabled: layoutNameTextField.text !== saveLayoutPopup.originName
                Layout.fillWidth: true
                onClicked: () => {
                    helper.addOn.saveCustomLayoutFromJavaScript(saveLayoutPopup.layout, saveLayoutPopup.originName, layoutNameTextField.text)
                    saveLayoutPopup.close()
                }
            }
        }
    }

    component DockingViewBindingHelper: QtObject {
        id: bindingHelper
        required property DockingView dockingView
        required property int firstKey
        required property int lastKey
        readonly property Binding binding: Binding {
            bindingHelper.dockingView.firstActive: !helper.activeUndockedPane && helper.activePanel === bindingHelper.firstKey
            bindingHelper.dockingView.lastActive: !helper.activeUndockedPane && helper.activePanel === bindingHelper.lastKey
            bindingHelper.dockingView.activeUndockedPane: helper.activeUndockedPane
        }
        readonly property Connections connections: Connections {
            target: bindingHelper.dockingView
            function onFirstActivated() {
                helper.activeUndockedPane = null
                helper.activePanel = bindingHelper.firstKey
            }
            function onLastActivated() {
                helper.activeUndockedPane = null
                helper.activePanel = bindingHelper.lastKey
            }
            function onUndockedActivated(pane) {
                helper.activeUndockedPane = pane
            }
            function onUndockedDeactivated(pane) {
                if (helper.activeUndockedPane === pane)
                    helper.activeUndockedPane = null
            }
        }
    }

    readonly property DockingViewBindingHelper leftBindingHelper: DockingViewBindingHelper {
        dockingView: helper.window.leftDockingView
        firstKey: WorkspaceAddOnHelper.LeftTop
        lastKey: WorkspaceAddOnHelper.LeftBottom

    }
    readonly property DockingViewBindingHelper rightBindingHelper: DockingViewBindingHelper {
        dockingView: helper.window.rightDockingView
        firstKey: WorkspaceAddOnHelper.RightTop
        lastKey: WorkspaceAddOnHelper.RightBottom

    }
    readonly property DockingViewBindingHelper topBindingHelper: DockingViewBindingHelper {
        dockingView: helper.window.topDockingView
        firstKey: WorkspaceAddOnHelper.TopLeft
        lastKey: WorkspaceAddOnHelper.TopRight

    }
    readonly property DockingViewBindingHelper bottomBindingHelper: DockingViewBindingHelper {
        dockingView: helper.window.bottomDockingView
        firstKey: WorkspaceAddOnHelper.BottomLeft
        lastKey: WorkspaceAddOnHelper.BottomRight

    }

}