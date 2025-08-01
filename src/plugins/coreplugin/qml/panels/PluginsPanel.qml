import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

DockingPane {
    id: pane
    title: qsTr("Plugins")
    PluginView {
        anchors.fill: parent
        useSplitView: false
    }
}