import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

ActionDockingPane {
    PluginView {
        anchors.fill: parent
        useSplitView: false
        onRestartRequested: CoreInterface.restartApplication()
    }
}