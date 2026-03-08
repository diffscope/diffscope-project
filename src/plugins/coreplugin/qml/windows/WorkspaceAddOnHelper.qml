import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

QtObject {
    id: helper
    required property QtObject addOn
    property ProjectWindowInterface windowHandle: addOn.windowHandle
    property Window window: windowHandle.window

    property DockingPane activeUndockedPane: null
    property int activePanel: -1

    readonly property DockingPane activeDockingPaneObject: {
        if (activeUndockedPane)
            return activeUndockedPane
        return dockingPanes[activePanel] ?? null
    }
    readonly property list<QtObject> dockingPanes: [
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

    readonly property LoggingCategory lcWorkspaceAddOnHelper: LoggingCategory {
        id: lcWorkspaceAddOnHelper
        name: "diffscope.core.qml.workspaceaddonhelper"
    }

    readonly property NotificationMessage noPanelNotification: NotificationMessage {
        id: noPanelNotification
        title: qsTr("The current workspace does not contain any panels")
        text: qsTr("Workspace data might be erroneous. You can try restoring the default workspace.")
        icon: SVS.Warning
        buttons: [qsTr("Restore Default Workspace")]
        property bool connectionEnabled: false
        onButtonClicked: () => {
            helper.addOn.workspaceManager.currentLayout = helper.addOn.workspaceManager.defaultLayout
            close()
        }
        onClosed: () => {
            connectionEnabled = false
        }
        readonly property Connections c: Connections {
            target: helper
            enabled: noPanelNotification.connectionEnabled
            function onAllPanesChanged() {
                if (helper.allPanes.length !== 0) {
                    noPanelNotification.close()
                }
            }

        }
    }


    function getIdHelper(object) {
        return object.ActionInstantiator.id ? object.ActionInstantiator.id : (object.actionId ?? "")

    }
    function initializeDockingViews() {
        console.info(lcWorkspaceAddOnHelper, "Initializing docking views")
        const conv = v => {
            if ((v.geometry?.width ?? 0) !== 0) {
                v.object.Docking.windowGeometryHint = v.geometry
            }
            return v.object
        }
        const loadState = v => {
            try {
                v.object.loadState(v.state)
                console.debug(lcWorkspaceAddOnHelper, "State loaded:", v.object, getIdHelper(v.object))
            } catch (e) {
            }
        }
        const f = (dockingView, edge) => {
            let o = addOn.createDockingViewContents(edge)
            o.objects.forEach(loadState)
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
        topData.objects.forEach(loadState)
        window.topDockingView.contentData = topData.objects.map(conv)
        window.topDockingView.splitterRatio = topData.splitterRatio
        bottomData.objects.forEach(loadState)
        window.bottomDockingView.contentData = bottomData.objects.map(conv)
        window.bottomDockingView.splitterRatio = bottomData.splitterRatio
        window.topDockingViewHeightRatio = topData.preferredPanelSize
        for (let i of topData.visibleIndices) {
            window.topDockingView.showPane(i)
        }
        for (let i of bottomData.visibleIndices) {
            window.bottomDockingView.showPane(i)
        }
        if (helper.allPanes.length === 0) {
            console.warn(lcWorkspaceAddOnHelper, "No panels found in current layout")
            noPanelNotification.connectionEnabled = true
            helper.windowHandle.sendNotification(noPanelNotification)
        }
    }

    function saveCurrentLayout() {
        console.info(lcWorkspaceAddOnHelper, "Saving current layout")
        savingCurrentLayout = true
        let viewSpecMap = []
        const saveState = o => {
            try {
                let state = o.saveState()
                console.debug(lcWorkspaceAddOnHelper, "State saved:", o, getIdHelper(o))
                return state
            } catch (e) {
                return undefined
            }
        }
        const f = (dockingView, isLeftOrRight, firstKey, lastKey) => {
            console.debug(lcWorkspaceAddOnHelper, "Saving docking view", firstKey);
            viewSpecMap[firstKey] = {
                panels: dockingView.contentData.slice(0, dockingView.stretchIndex).map(o => {
                    console.debug(lcWorkspaceAddOnHelper, "Handling panel", o, getIdHelper(o))
                    let v = {
                        id: getIdHelper(o),
                        dock: o.dock,
                        opened: o.Docking.window?.visible ?? false,
                        geometry: Qt.rect(o.Docking.window?.x ?? 0, o.Docking.window?.y ?? 0, o.Docking.window?.width ?? 0, o.Docking.window?.height ?? 0),
                        data: saveState(o)
                    }
                    console.debug(lcWorkspaceAddOnHelper, "Handled panel spec",o , v.id, v.dock, v.opened, v.geometry, v.data)
                    return v
                }),
                width: isLeftOrRight ? dockingView.preferredPanelSize : dockingView.splitterRatio,
                height: isLeftOrRight ? dockingView.splitterRatio : window.topDockingViewHeightRatio,
                visibleIndex: dockingView.firstIndex,
            }
            console.debug(lcWorkspaceAddOnHelper, "Saved docking view", firstKey, viewSpecMap[firstKey].width, viewSpecMap[firstKey].height, viewSpecMap[firstKey].visibleIndex)
            console.debug(lcWorkspaceAddOnHelper, "Saving docking view", lastKey)
            viewSpecMap[lastKey] = {
                panels: dockingView.contentData.slice(dockingView.stretchIndex + 1).map(o => {
                    console.debug(lcWorkspaceAddOnHelper, "Handling panel", o, getIdHelper(o))
                    let v = {
                        id: getIdHelper(o),
                        dock: o.dock,
                        opened: o.Docking.window?.visible ?? false,
                        geometry: Qt.rect(o.Docking.window?.x ?? 0, o.Docking.window?.y ?? 0, o.Docking.window?.width ?? 0, o.Docking.window?.height ?? 0),
                        data: saveState(o)
                    }
                    console.debug(lcWorkspaceAddOnHelper, "Handled panel spec",o , v.id, v.dock, v.opened, v.geometry, v.data)
                    return v
                }),
                width: isLeftOrRight ? dockingView.panelSize : 1 - dockingView.splitterRatio,
                height: isLeftOrRight ? 1 - dockingView.splitterRatio :  window.topDockingViewHeightRatio,
                visibleIndex: dockingView.lastIndex - (dockingView.stretchIndex + 1),
            }
            console.debug(lcWorkspaceAddOnHelper, "Saved docking view", lastKey, viewSpecMap[lastKey].width, viewSpecMap[lastKey].height, viewSpecMap[lastKey].visibleIndex)
        }
        f(window.leftDockingView, true, WorkspaceAddOnHelper.LeftTop, WorkspaceAddOnHelper.LeftBottom)
        f(window.rightDockingView, true, WorkspaceAddOnHelper.RightTop, WorkspaceAddOnHelper.RightBottom)
        f(window.topDockingView, false, WorkspaceAddOnHelper.TopLeft, WorkspaceAddOnHelper.TopRight)
        f(window.bottomDockingView, false, WorkspaceAddOnHelper.BottomLeft, WorkspaceAddOnHelper.BottomRight)
        addOn.workspaceManager.currentLayout.setViewSpecFromJavaScript(viewSpecMap)
        savingCurrentLayout = false
    }

    function cleanupPanes() {
        console.info(lcWorkspaceAddOnHelper, "Cleaning up panes")
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
        console.info(lcWorkspaceAddOnHelper, "Prompting to save layout")
        let newName = helper.windowHandle.execQuickInput(qsTr("Custom layout name"), "", originName, (text, attempted) => {
            if (text === "") {
                return {
                    acceptable: false,
                    status: attempted ? SVS.CT_Error : SVS.CT_Normal,
                    promptText: attempted ? qsTr("Name should not be empty") : ""
                }
            }
            if (text === originName) {
                return {
                    acceptable: false,
                    status: SVS.CT_Error,
                    promptText: qsTr("New name should not be the same as old name")
                }
            }
            if (helper.addOn.workspaceManager.customLayouts.some(o => o.name === text)) {
                return {
                    acceptable: true,
                    status: SVS.CT_Warning,
                    promptText: qsTr("Custom presets with the same name will be overwritten")
                }
            }
            return {
                acceptable: true,
                status: SVS.CT_Normal,
                promptText: ""
            }
        })
        if (typeof(newName) === "string") {
            console.info(lcWorkspaceAddOnHelper, "Saving layout", newName)
            helper.addOn.saveCustomLayoutFromJavaScript(layout, originName, newName)
        } else {
            console.info(lcWorkspaceAddOnHelper, "Save layout canceled")
        }
    }

    function promptAddAction(action) {
        console.info(lcWorkspaceAddOnHelper, "Prompting to add action")
        newActionPopup.actionObject = action
        newActionPopup.open()
    }

    function promptDeleteLayout(name) {
        console.info(lcWorkspaceAddOnHelper, "Prompting to delete layout")
        if (window.contentItem.MessageBox.question(qsTr("Delete"), qsTr('Delete layout "%1"?').arg(name)) === SVS.Yes) {
            addOn.removeCustomLayoutFromJavaScript(name)
        }
    }

    readonly property Connections windowHandleConnections: Connections {
        target: helper.addOn
        function onWindowExposed() {
            Qt.callLater(() => helper.initializeDockingViews())
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

    readonly property Popup newActionPopup: Popup {
        id: newActionPopup
        anchors.centerIn: parent
        padding: 16
        parent: helper.window.contentItem
        width: 400
        property QtObject actionObject: null
        ColumnLayout {
            anchors.fill: parent
            spacing: 16
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr('Drag to the sidebar to add "%1"').arg(newActionPopup.actionObject?.ActionInstantiator.text ?? "")
            }
            Frame {
                Layout.alignment: Qt.AlignHCenter
                ThemedItem.backgroundLevel: SVS.BL_Quaternary
                width: 40
                height: 40
                Item {
                    id: dragTarget
                    readonly property QtObject modelData: newActionPopup.actionObject
                    readonly property QtObject container: null
                    width: 24
                    height: 24
                    component DragIconLabel: IconLabel {
                        anchors.fill: parent
                        icon.source: newActionPopup.actionObject?.ActionInstantiator.icon.source ?? ""
                        icon.color: newActionPopup.actionObject?.ActionInstantiator.icon.color.valid ? newActionPopup.actionObject.ActionInstantiator.icon.color : Theme.foregroundPrimaryColor
                        icon.width: 16
                        icon.height: 16
                        font: Theme.font
                        display: AbstractButton.IconOnly
                    }
                    DragIconLabel {
                    }
                    Popup {
                        background: Item {}
                        padding: 0
                        width: parent.width
                        height: parent.height
                        visible: mouseArea.drag.active
                        DragIconLabel {
                        }
                    }
                    anchors.centerIn: mouseArea.drag.active ? null : parent
                    Drag.keys: ["SVSCraft.UIComponent.DockingView"]
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2
                    Drag.active: mouseArea.drag.active
                    function remove() {
                        newActionPopup.actionObject = null
                        newActionPopup.close()
                    }
                }
                MouseArea {
                    id: mouseArea
                    drag.target: dragTarget
                    width: 24
                    height: 24
                    anchors.centerIn: parent
                    onReleased: () => {
                        if (!drag.active)
                            return
                        if (dragTarget.Drag.target) {
                            dragTarget.Drag.drop()
                        }
                    }
                }
            }
        }
        onClosed: () => {
            if (actionObject) {
                actionObject.destroy()
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
                if (helper.activePanel === bindingHelper.firstKey && !helper.activeUndockedPane)
                    return
                console.debug(lcWorkspaceAddOnHelper, "Docking panel activated", bindingHelper.firstKey)
                helper.activeUndockedPane = null
                helper.activePanel = bindingHelper.firstKey
            }
            function onLastActivated() {
                if (helper.activePanel === bindingHelper.lastKey && !helper.activeUndockedPane)
                    return
                console.debug(lcWorkspaceAddOnHelper, "Docking panel activated", bindingHelper.lastKey)
                helper.activeUndockedPane = null
                helper.activePanel = bindingHelper.lastKey
            }
            function onUndockedActivated(pane) {
                console.debug(lcWorkspaceAddOnHelper, "Undocked panel activated", pane, pane.title)
                helper.activeUndockedPane = pane
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