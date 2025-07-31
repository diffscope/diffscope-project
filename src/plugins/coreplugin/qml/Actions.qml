import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.CorePlugin

QtObject {
    readonly property Component newFile: Action {
        onTriggered: ICore.newFile()
    }
    readonly property Component openFile: Action {

    }
    readonly property Component settings: Action {
        onTriggered: (o) => {
            let w = o.Window.window
            Qt.callLater(() => ICore.showSettingsDialog("", w))
        }
    }
    readonly property Component plugins: Action {
        onTriggered: (o) => {
            let w = o.Window.window
            Qt.callLater(() => ICore.showPluginsDialog(w))
        }
    }
    readonly property Component showHome: Action {
        onTriggered: () => {
            ICore.showHome()
        }
    }
    readonly property Component workspaceLayouts: Menu {
        title: ActionInstantiator.text
        icon.source: ActionInstantiator.iconSource
        Action {
            text: qsTr("Default")
        }
        Menu {
            title: qsTr("Custom Layouts")
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Manage Layouts...")
        }
    }
    readonly property Component workspacePanels: Menu {
        title: ActionInstantiator.text
        icon.source: ActionInstantiator.iconSource
    }
    readonly property Component dockActionToSideBar: Action {

    }

    readonly property Component documentations: Action {

    }
    readonly property Component findActions: Action {

    }
    readonly property Component aboutApp: Action {
        onTriggered: (o) => {
            let w = o.Window.window
            Qt.callLater(() => ICore.showAboutAppDialog(w))
        }
    }
    readonly property Component aboutQt: Action {
        onTriggered: (o) => {
            let w = o.Window.window
            Qt.callLater(() => ICore.showAboutQtDialog(w))
        }
    }
}