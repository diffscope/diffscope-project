import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

ActionCollection {
    id: d

    required property ViewVisibilityAddOn addOn
    readonly property Window window: addOn?.windowHandle.window ?? null

    ActionItem {
        actionId: "core.view.showMenuBar"
        Action {
            checkable: true
            checked: d.window.menuBar.visible
            onTriggered: () => {
                Qt.callLater(() => d.addOn.toggleVisibility(ViewVisibilityAddOn.MenuBar, checked, this))
            }
        }
    }

    ActionItem {
        actionId: "core.view.showToolBar"
        Action {
            checkable: true
            checked: d.window.toolBar.visible
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.ToolBar, checked)
            }
        }
    }

    ActionItem {
        actionId: "core.view.showLeftSideBar"
        Action {
            checkable: true
            checked: d.window.leftDockingView.barSize !== 0
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.LeftSideBar, checked)
            }
        }
    }

    ActionItem {
        actionId: "core.view.showRightSideBar"
        Action {
            checkable: true
            checked: d.window.rightDockingView.barSize !== 0
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.RightSideBar, checked, this)
            }
        }
    }

    ActionItem {
        actionId: "core.view.showTopSideBar"
        Action {
            checkable: true
            checked: d.window.topDockingView.barSize !== 0
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.TopSideBar, checked)
            }
        }
    }

    ActionItem {
        actionId: "core.view.showBottomSideBar"
        Action {
            checkable: true
            checked: d.window.bottomDockingView.barSize !== 0
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.BottomSideBar, checked)
            }
        }
    }

    ActionItem {
        actionId: "core.view.showStatusBar"
        Action {
            checkable: true
            checked: d.window.statusBar.visible
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.StatusBar, checked)
            }
        }
    }

}