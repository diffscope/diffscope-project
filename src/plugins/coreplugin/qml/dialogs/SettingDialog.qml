// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick

import SVSCraft.Extras

import ChorusKit.AppCore

import DiffScope.UIShell
import DiffScope.Core

SettingDialog {
    id: dialog

    WindowSystem.windowSystem: CoreInterface.windowSystem
    WindowSystem.id: "org.diffscope.core.settingdialog"

    settingCatalog: CoreInterface.settingCatalog
    Settings {
        settings: RuntimeInterface.state
        category: "org.diffscope.core.settingDialog"
        property alias navigationWidth: dialog.navigationWidth
        property alias currentId: dialog.currentId
    }
}