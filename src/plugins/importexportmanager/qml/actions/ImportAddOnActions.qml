import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents

import QActionKit

import DiffScope.UIShell

ActionCollection {
    id: d

    required property QtObject addOn

    ActionItem {
        actionId: "org.diffscope.importexportmanager.file.import"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.execImport())
        }
    }
}