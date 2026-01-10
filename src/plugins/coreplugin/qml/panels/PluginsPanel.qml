import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ChorusKit.AppCore

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

ActionDockingPane {
    PluginView {
        anchors.fill: parent
        useSplitView: false
        onRestartRequested: RuntimeInterface.restartApplication()
    }
}