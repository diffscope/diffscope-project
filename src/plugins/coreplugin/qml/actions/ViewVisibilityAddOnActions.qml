import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

ActionCollection {
    id: d

    required property ViewVisibilityAddOn addOn
    readonly property Window window: addOn?.windowHandle.window ?? null

    ActionItem {
        actionId: "core.menuBarVisible"
        Action {
            checkable: true
            checked: d.window.menuBar.visible
            onTriggered: () => {
                Qt.callLater(() => d.addOn.toggleVisibility(ViewVisibilityAddOn.MenuBar, checked, this))
            }
        }
    }

    ActionItem {
        actionId: "core.toolBarVisible"
        Action {
            checkable: true
            checked: d.window.toolBar.visible
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.ToolBar, checked)
            }
        }
    }

    ActionItem {
        actionId: "core.leftSideBarVisible"
        Action {
            checkable: true
            checked: d.window.leftDockingView.barSize !== 0
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.LeftSideBar, checked)
            }
        }
    }

    ActionItem {
        actionId: "core.rightSideBarVisible"
        Action {
            checkable: true
            checked: d.window.rightDockingView.barSize !== 0
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.RightSideBar, checked, this)
            }
        }
    }

    ActionItem {
        actionId: "core.topSideBarVisible"
        Action {
            checkable: true
            checked: d.window.topDockingView.barSize !== 0
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.TopSideBar, checked)
            }
        }
    }

    ActionItem {
        actionId: "core.bottomSideBarVisible"
        Action {
            checkable: true
            checked: d.window.bottomDockingView.barSize !== 0
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.BottomSideBar, checked)
            }
        }
    }

    ActionItem {
        actionId: "core.statusBarVisible"
        Action {
            checkable: true
            checked: d.window.statusBar.visible
            onTriggered: () => {
                d.addOn.toggleVisibility(ViewVisibilityAddOn.StatusBar, checked)
            }
        }
    }

}