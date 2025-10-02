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
        settings: RuntimeInterface.settings
        category: "DiffScope.Core.SettingDialog"
        property alias navigationWidth: dialog.navigationWidth
        property alias currentId: dialog.currentId
    }
}