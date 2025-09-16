import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: d

    required property HomeWindowInterface windowHandle
    property Window window: windowHandle?.window ?? null

    ActionItem {
        actionId: "core.home.recentFiles"
        Action {
            checkable: true
            checked: !d.window.recoveryFilesVisible
            onTriggered: () => {
                d.window.recoveryFilesVisible = false
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "core.home.recoveryFiles"
        Action {
            checkable: true
            checked: d.window.recoveryFilesVisible
            onTriggered: () => {
                d.window.recoveryFilesVisible = true
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "core.home.gridView"
        Action {
            enabled: !d.window.recoveryFilesVisible
            checkable: true
            checked: !d.window.recentFilesIsListView
            onTriggered: () => {
                d.window.recentFilesIsListView = false
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

    ActionItem {
        actionId: "core.home.listView"
        Action {
            enabled: !d.window.recoveryFilesVisible
            checkable: true
            checked: d.window.recentFilesIsListView
            onTriggered: () => {
                d.window.recentFilesIsListView = true
                Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
            }
        }
    }

}
