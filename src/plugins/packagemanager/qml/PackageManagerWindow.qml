import QtQml
import QtQuick

import ChorusKit.AppCore

import DiffScope.Core
import DiffScope.PackageManager
import DiffScope.UIShell

Window {
    id: window

    required property PackageManagerAddOn addOn

    width: 1080
    height: 640
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    title: qsTr("Package Manager")

    WindowSystem.windowSystem: CoreInterface.windowSystem
    WindowSystem.id: "org.diffscope.packagemanager.packagemanagerwindow"

    signal finished()
    onClosing: finished()
    Component.onCompleted: Qt.callLater(() => addOn.refresh())

    PackageManagerView {
        id: packageManagerView
        anchors.fill: parent
        model: window.addOn.packageModel
        refreshing: window.addOn.refreshing

        onRefreshRequested: window.addOn.refresh()
    }

    Connections {
        target: window.addOn
        function onRefreshStarted() {
            packageManagerView.currentIndex = window.addOn.invalidIndex()
        }
    }
}
