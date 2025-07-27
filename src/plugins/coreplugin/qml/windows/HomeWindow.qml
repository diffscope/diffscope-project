import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import QActionKit

import DiffScope.UIShell
import DiffScope.CorePlugin

HomeWindow {
    id: homeWindow
    required property QtObject windowHandle
    navigationActionsModel: ObjectModel {
        property ActionInstantiator instantiator: ActionInstantiator {
            actionId: "core.homeNavigation"
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
        Menu {
            title: qsTr("Preferences")
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
            Action {
                text: qsTr("Settings...")
                onTriggered: ICore.showSettingsDialog("core.Appearance", homeWindow)
            }
            Action {
                text: qsTr("Plugins...")
                onTriggered: ICore.showPluginsDialog(homeWindow)
            }
        }
        Menu {
            title: qsTr("About")
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
            Action {
                text: qsTr("About %1").replace("%1", Application.name)
            }
            Action {
                text: qsTr("About Qt")
            }
        }
    }
}