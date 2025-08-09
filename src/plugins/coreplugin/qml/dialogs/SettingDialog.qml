import QtQml
import QtQuick

import SVSCraft.Extras

import ChorusKit.AppCore

import DiffScope.UIShell
import DiffScope.CorePlugin

SettingDialog {
    id: dialog
    settingCatalog: ICore.settingCatalog
    Settings {
        settings: PluginDatabase.settings
        category: "DiffScope.CorePlugin.SettingDialog"
        property alias navigationWidth: dialog.navigationWidth
        property alias currentId: dialog.currentId
    }
}