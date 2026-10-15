// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import SVSCraft.UIComponents

import DiffScope.Core

ActionCollection {
    id: d

    required property ViewVisibilityAddOn addOn
    readonly property Window window: addOn?.windowHandle.window ?? null

    ActionItem {
        actionId: "org.diffscope.core.view.showMenuBar"
        readonly property Component _actionComponent: Action {
            checkable: true
            checked: d.window.menuBar.alwaysVisible
            enabled: BehaviorPreference.projectWindowTitleBarStyle !== BehaviorPreference.TS_MacOS
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.MenuBar, checked)
            }
        }
        actionComponent: (Qt.platform.os !== "osx" && Qt.platform.os !== "macos") ? _actionComponent : null
    }

    ActionItem {
        actionId: "org.diffscope.core.view.showToolBar"
        Action {
            checkable: true
            checked: d.window.toolBar.visible
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.ToolBar, checked)
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.view.showStatusBar"
        Action {
            checkable: true
            checked: d.window.statusBar.visible
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.StatusBar, checked)
            }
        }
    }

}
