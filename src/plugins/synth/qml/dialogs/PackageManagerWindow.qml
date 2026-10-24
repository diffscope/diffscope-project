// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick

import ChorusKit.AppCore

import DiffScope.Core
import DiffScope.Synth
import DiffScope.UIShell

Window {
    id: window

    required property PackageManagerController packageManager

    width: 1080
    height: 640
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    title: qsTr("Package Manager")

    WindowSystem.windowSystem: CoreInterface.windowSystem
    WindowSystem.id: "org.diffscope.synth.packagemanagerwindow"

    signal finished()
    onClosing: (event) => {
        if (packageManager.uninstalling || packageManager.installing)
            event.accepted = false
        else
            finished()
    }
    Component.onCompleted: Qt.callLater(() => packageManager.refresh())

    PackageManagerView {
        id: packageManagerView
        anchors.fill: parent
        enabled: !window.packageManager.packageOperationActive
        model: window.packageManager.packageModel
        refreshing: window.packageManager.refreshing
        uninstalling: window.packageManager.uninstalling
        installing: window.packageManager.installing

        onRefreshRequested: window.packageManager.refresh()
        onUninstallRequested: (index) => window.packageManager.uninstallPackage(index)
        onInstallRequested: packageFileDialog.open()
    }

    PackageFileDialog {
        id: packageFileDialog
        onAccepted: window.packageManager.installPackages(selectedFiles)
    }

    Connections {
        target: window.packageManager
        function onRefreshStarted() {
            packageManagerView.currentIndex = window.packageManager.invalidIndex()
        }
    }
}
