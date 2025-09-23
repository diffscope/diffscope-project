import QtQml
import QtQuick

import SVSCraft.Extras

import ChorusKit.AppCore

import DiffScope.UIShell
import DiffScope.Core

SettingDialog {
    id: dialog
    settingCatalog: CoreInterface.settingCatalog
    Settings {
        settings: RuntimeInterface.settings
        category: "DiffScope.Core.SettingDialog"
        property alias navigationWidth: dialog.navigationWidth
        property alias currentId: dialog.currentId
    }
}