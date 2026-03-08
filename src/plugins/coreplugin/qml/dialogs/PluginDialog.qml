import QtQml
import QtQuick

import ChorusKit.AppCore

import DiffScope.UIShell
import DiffScope.Core

Window {
    width: 800
    height: 600
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    title: qsTr("Plugins")

    WindowSystem.windowSystem: CoreInterface.windowSystem
    WindowSystem.id: "org.diffscope.core.plugindialog"

    signal finished()
    onClosing: finished()
    PluginView {
        anchors.fill: parent
        onRestartRequested: RuntimeInterface.restartApplication()
    }
    // TODO: load/save window and splitter size
}