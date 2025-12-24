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
        actionId: "org.diffscope.core.home.navigationPanels"
        Menu {
            id: menu
            Instantiator {
                model: DelegateModel {
                    model: window.panelsModel.count
                    delegate: Action {
                        required property int index
                        checkable: true
                        checked: window.currentNavIndex === index
                        text: window.panelsModel.get(index).title
                        onTriggered: () => {
                            window.currentNavIndex = index
                            Qt.callLater(() => GlobalHelper.setProperty(this, "checked", true))
                        }
                    }
                }
                onObjectAdded: (index, object) => {
                    menu.insertAction(index, object)
                }
                onObjectRemoved: (index, object) => {
                    menu.removeAction(object)
                }
            }
        }
    }

}
