import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import DiffScope.UIShell
import DiffScope.CorePlugin

HomeWindow {
    // TODO: temporarily put actions here before QActionKit is integrated into this project
    id: homeWindow
    toolActionsModel: ObjectModel {
        Action {
            text: "Settings..."
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
            onTriggered: ICore.showSettingsDialog("core.Appearance", homeWindow)
        }
        Action {
            text: "Plugins..."
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
            onTriggered: ICore.showPluginsDialog(homeWindow)
        }
    }
}