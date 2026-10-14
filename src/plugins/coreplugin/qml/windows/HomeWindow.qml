// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import SVSCraft.UIComponents
import SVSCraft.Extras

import QActionKit

import ChorusKit.AppCore

import DiffScope.UIShell
import DiffScope.Core

HomeWindow {
    id: homeWindow
    required property HomeWindowInterface windowHandle
    required property QtObject notificationViewModel
    frameless: BehaviorPreference.uiBehavior & BehaviorPreference.UB_Frameless
    readonly property color lightBannerColor: "#dadada"
    readonly property color darkBannerColor: "#252525"
    banner: {
        let c = ColorUtils.selectHighestContrastColor(Theme.backgroundPrimaryColor, [lightBannerColor, darkBannerColor]);
        if (c.r < 0.5) {
            return "qrc:/diffscope/coreplugin/logos/BannerDark.png";
        } else {
            return "qrc:/diffscope/coreplugin/logos/BannerLight.png";
        }
    }

    WindowSystem.windowSystem: CoreInterface.windowSystem
    WindowSystem.id: "org.diffscope.core.homewindow"

    NotificationBubbleModel {
        id: notificationBubbleModel
        notificationModel: homeWindow.notificationViewModel
    }
    bubbleNotificationsModel: notificationBubbleModel.model
    notificationEnablesAnimation: true

    onNewFileRequested: () => {
        windowHandle.triggerAction("org.diffscope.core.file.new", homeWindow.contentItem)
    }
    property var panelStateCache: settings.value("panelData") ?? []
    property bool rebuildingPanels: false
    property var activePanelKey: null

    function panelKey(object, index) {
        const id = object.ActionInstantiator.id
        let occurrence = 0
        for (let i = 0; i < index; ++i) {
            if (panelsModel.get(i).ActionInstantiator.id === id)
                ++occurrence
        }
        return {id, occurrence}
    }
    function savePanelState(object, index) {
        const key = panelKey(object, index)
        const entry = {id: key.id, occurrence: key.occurrence, state: object.saveState ? object.saveState() : null}
        const list = panelStateCache.slice()
        const oldIndex = list.findIndex(o => o.id === key.id && o.occurrence === key.occurrence)
        if (oldIndex >= 0)
            list[oldIndex] = entry
        else
            list.push(entry)
        panelStateCache = list
    }
    function finishPanelUpdate() {
        if (activePanelKey) {
            for (let i = 0; i < panelsModel.count; ++i) {
                const key = panelKey(panelsModel.get(i), i)
                if (key.id === activePanelKey.id && key.occurrence === activePanelKey.occurrence) {
                    currentNavIndex = i
                    break
                }
            }
        }
        rebuildingPanels = false
        activePanelKey = null
        updateCurrentPanel()
    }
    panelsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "org.diffscope.core.homePanels"
            context: homeWindow.windowHandle.actionContext
            onObjectAdded: (index, object) => {
                homeWindow.panelsModel.insert(index, object)
                const key = homeWindow.panelKey(object, index)
                const stateData = homeWindow.panelStateCache.find(o => o.id === key.id && o.occurrence === key.occurrence)
                if (stateData && object.loadState)
                    object.loadState(stateData.state)
                Qt.callLater(homeWindow.finishPanelUpdate)
            }
            onObjectRemoved: (index, object) => {
                if (!homeWindow.rebuildingPanels) {
                    homeWindow.rebuildingPanels = true
                    const activeIndex = homeWindow.currentNavIndex
                    if (activeIndex >= 0 && activeIndex < homeWindow.panelsModel.count)
                        homeWindow.activePanelKey = homeWindow.panelKey(homeWindow.panelsModel.get(activeIndex), activeIndex)
                }
                homeWindow.savePanelState(object, index)
                homeWindow.panelsModel.remove(index)
                Qt.callLater(homeWindow.finishPanelUpdate)
            }
        }
    }
    navigationActionsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "org.diffscope.core.homeNavigation"
            context: homeWindow.windowHandle.actionContext
            onObjectAdded: (index, object) => {
                homeWindow.navigationActionsModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                homeWindow.navigationActionsModel.remove(index)
            }
        }
    }
    toolActionsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "org.diffscope.core.homeTool"
            context: homeWindow.windowHandle.actionContext
            onObjectAdded: (index, object) => {
                homeWindow.toolActionsModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                homeWindow.toolActionsModel.remove(index)
            }
        }
    }
    menusModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "org.diffscope.core.homeMenu"
            context: homeWindow.windowHandle.actionContext
            onObjectAdded: (index, object) => {
                homeWindow.menusModel.insert(index, object)
            }
            onObjectRemoved: (index, object) => {
                homeWindow.menusModel.remove(index)
            }
        }
    }

    Settings {
        id: settings
        settings: RuntimeInterface.settings
        category: "DiffScope.Core.HomeWindow"
        property alias recentFilesIsListView: homeWindow.recentFilesIsListView
    }

    onClosing: () => {
        let a = []
        for (let i = 0; i < homeWindow.panelsModel.count; i++) {
            let pane = homeWindow.panelsModel.get(i)
            let state = pane.saveState ? pane.saveState() : null
            a.push({
                id: pane.ActionInstantiator.id,
                occurrence: homeWindow.panelKey(pane, i).occurrence,
                state
            })
        }
        settings.setValue("panelData", a)
    }
}
