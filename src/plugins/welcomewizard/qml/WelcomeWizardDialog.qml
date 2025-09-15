import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents

import DiffScope.UIShell

WelcomeWizardDialog {
    readonly property color lightBannerColor: "#dadada"
    readonly property color darkBannerColor: "#252525"
    banner: {
        let c = ColorUtils.selectHighestContrastColor(Theme.backgroundPrimaryColor, [lightBannerColor, darkBannerColor]);
        if (c.r < 0.5) {
            return "qrc:/diffscope/coreplugin/logos/BannerDark.png";
        } else {
            return "qrc:/diffscope/coreplugin/logos/BannerLight.png";
        }
    }
}