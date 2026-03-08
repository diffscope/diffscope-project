import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: d
    required property QtObject addOn

    ActionItem {
        actionId: "org.diffscope.core.window.nextProjectWindow"
        Action {
            enabled: d.addOn.projectWindows.length !== 0
            onTriggered: d.addOn.navigateToWindow(1)
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.window.previousProjectWindow"
        Action {
            enabled: d.addOn.projectWindows.length !== 0
            onTriggered: d.addOn.navigateToWindow(-1)
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.window.projectWindows"
        Menu {
            id: menu
            enabled: d.addOn.projectWindows.length !== 0
            Instantiator {
                model: DelegateModel {
                    model: d.addOn.projectWindows
                    delegate: Action {
                        required property int index
                        required property ProjectWindowInterface modelData
                        text: (index < 9 ? `&${Qt.locale().toString(index + 1)}. ` : "") + modelData.window.documentName
                        checkable: true
                        checked: d.addOn.windowHandle === modelData
                        onTriggered: () => {
                            d.addOn.raiseWindow(modelData)
                            checked = d.addOn.windowHandle === modelData
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