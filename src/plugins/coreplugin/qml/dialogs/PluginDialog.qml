import QtQml
import QtQuick

import DiffScope.UIShell
import DiffScope.CorePlugin

Window {
    width: 800
    height: 600
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    title: qsTr("Plugins")
    signal finished()
    onClosing: finished()
    PluginView {
        anchors.fill: parent
    }
    // TODO: load/save window and splitter size
}