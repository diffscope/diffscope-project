// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

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
    onClosing: (event) => {
        if (addOn.uninstalling || addOn.installing)
            event.accepted = false
        else
            finished()
    }
    Component.onCompleted: Qt.callLater(() => addOn.refresh())

    PackageManagerView {
        id: packageManagerView
        anchors.fill: parent
        model: window.addOn.packageModel
        refreshing: window.addOn.refreshing
        uninstalling: window.addOn.uninstalling
        installing: window.addOn.installing

        onRefreshRequested: window.addOn.refresh()
        onUninstallRequested: (index) => window.addOn.uninstallPackage(index)
        onInstallRequested: packageFileDialog.open()
    }

    PackageFileDialog {
        id: packageFileDialog
        onAccepted: window.addOn.installPackages(selectedFiles)
    }

    Connections {
        target: window.addOn
        function onRefreshStarted() {
            packageManagerView.currentIndex = window.addOn.invalidIndex()
        }
    }
}
