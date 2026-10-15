// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

QtObject {
    id: helper
    required property QtObject addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property Window window: windowHandle?.window ?? null
    readonly property QtObject session: addOn?.session ?? null
    readonly property bool usable: Boolean(addOn && windowHandle && window && session && !session.shuttingDown)

    readonly property DockingView workspaceView: DockingView {
        anchors.fill: parent
        host: helper.session?.embeddedHost ?? null
        enabled: helper.usable && !helper.session.busy
        automaticFloatingWindows: false
    }
    Component.onCompleted: if (window)
        window.workspaceContent.push(workspaceView)

    readonly property NotificationMessage noPanelNotification: NotificationMessage {
        title: qsTr("The current workspace does not contain any panels")
        text: qsTr("Workspace data might be erroneous. You can try restoring the default workspace.")
        icon: SVS.Warning
        buttons: [qsTr("Restore Default Workspace")]
        onButtonClicked: if (helper.usable)
            helper.addOn.workspaceManager.currentLayout = helper.addOn.workspaceManager.defaultLayout
    }
    property bool noPanelNotificationVisible: false

    function updateEmptyWarning() {
        if (!usable || session.busy || !session.ready)
            return
        if (session.context.placedPanelCount === 0 && !noPanelNotificationVisible) {
            noPanelNotificationVisible = true
            windowHandle.sendNotification(noPanelNotification)
        } else if (session.context.placedPanelCount !== 0 && noPanelNotificationVisible) {
            noPanelNotification.close()
            noPanelNotificationVisible = false
        }
    }

    readonly property Connections sessionConnections: Connections {
        target: helper.session

        function onRestored() {
            helper.updateEmptyWarning()
            Qt.callLater(helper.presentNewPanels)
        }

        function onErrorOccurred(message) {
            if (helper.usable)
                helper.windowHandle.sendNotification(SVS.Critical, qsTr("Workspace Error"), message)
        }

        function onShuttingDownChanged() {
            if (!helper.session?.shuttingDown)
                return
            helper.newPanels.close()
            if (helper.noPanelNotificationVisible) {
                helper.noPanelNotification.close()
                helper.noPanelNotificationVisible = false
            }
        }
    }

    readonly property Connections contextConnections: Connections {
        target: helper.session?.context ?? null

        function onChanged() {
            helper.updateEmptyWarning()
        }
    }

    function presentNewPanels() {
        if (usable && session.ready && !session.busy && window.visible && window.visibility !== Window.Minimized && addOn.newPanelEntries.length && !newPanels.visible)
            newPanels.open()
    }

    readonly property Connections windowConnections: Connections {
        target: helper.window

        function onVisibleChanged() {
            Qt.callLater(helper.presentNewPanels)
        }

        function onVisibilityChanged() {
            Qt.callLater(helper.presentNewPanels)
        }
    }

    function promptSaveLayout(layout, originName) {
        if (!usable)
            return
        const name = windowHandle.execQuickInput(qsTr("Custom Layout Name"), "", originName, (text, attempted) => {
            if (!helper.usable)
                return {
                    acceptable: false
                }
            if (!text)
                return {
                    acceptable: false,
                    status: attempted ? SVS.CT_Error : SVS.CT_Normal,
                    promptText: attempted ? qsTr("Name should not be empty") : ""
                }
            if (text === originName)
                return {
                    acceptable: false,
                    status: SVS.CT_Error,
                    promptText: qsTr("New name should not be the same as old name")
                }
            if (addOn.workspaceManager.customLayouts.some(layout => layout.name === text))
                return {
                    acceptable: true,
                    status: SVS.CT_Warning,
                    promptText: qsTr("Custom presets with the same name will be overwritten")
                }
            return {
                acceptable: true,
                status: SVS.CT_Normal,
                promptText: ""
            }
        })
        if (usable && typeof name === "string")
            addOn.saveCustomLayoutFromJavaScript(layout, originName, name)
    }

    function promptDeleteLayout(name) {
        if (!usable)
            return
        if (window.contentItem.MessageBox.question(qsTr("Delete Layout"), qsTr('Delete layout "%1"?').arg(name)) === SVS.Yes && usable)
            addOn.removeCustomLayoutFromJavaScript(name)
    }

    readonly property NewPanelsDialog newPanels: NewPanelsDialog {
        parent: helper.window?.contentItem ?? null
        anchors.centerIn: parent
        addOn: helper.addOn
    }
}
