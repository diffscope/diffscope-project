import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents

import QActionKit

import DiffScope.UIShell

ActionCollection {

    required property QtObject addOn

    ActionItem {
        actionId: "org.diffscope.importexportmanager.file.export"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.importexportmanager.project.importAsTracks"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.importexportmanager.edit.copySpecial"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.importexportmanager.edit.pasteSpecial"
        Action {
            onTriggered: () => {

            }
        }
    }
}