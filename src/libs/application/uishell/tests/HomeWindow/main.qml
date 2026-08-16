// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQml.Models

import SVSCraft

import DiffScope.UIShell

HomeWindow {
    id: window
    visible: true
    banner: "qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_logo.png"
    navigationActionsModel: ObjectModel {
        Action {
            text: "New"
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Action {
            text: "Open..."
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Action {
            text: "Import..."
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Action {
            text: "Custom Action"
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Menu {
            title: "Custom Menu"
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
            Action {
                text: "aaa"
            }
            Action {
                text: "bbb"
            }
        }
    }
    toolActionsModel: ObjectModel {
        Action {
            text: "Settings..."
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Action {
            text: "Plugins..."
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
        Menu {
            title: "Help"
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
            Action { text: "Help" }
            Action { text: "About" }
        }
    }

    bubbleNotificationsModel: ObjectModel {
        id: notificationModel
        QtObject {
            property string title: "Background task"
            property string text: "Rendering preview..."
            property int icon: SVS.Information
            property list<string> buttons: ["Open"]
            property int primaryButton: 0
            property bool closable: true
            property bool hasProgress: true
            property double progress: 0.65
            property bool progressAbortable: true
            property bool permanentlyHideable: false
            property int textFormat: Text.AutoText
            function hideClicked() { notificationModel.remove(ObjectModel.index) }
            function closeClicked() { notificationModel.remove(ObjectModel.index) }
            function abortClicked() { console.log("abort") }
            function permanentlyHideClicked() {}
            function buttonClicked(index) { console.log("button", index) }
            function hoverEntered() {}
            function hoverExited() {}
            function linkActivated(link) { console.log(link) }
        }
        QtObject {
            property string title: "Warning"
            property string text: "This is a HomeWindow notification smoke test."
            property int icon: SVS.Warning
            property list<string> buttons: []
            property int primaryButton: -1
            property bool closable: true
            property bool hasProgress: false
            property double progress: 0
            property bool progressAbortable: false
            property bool permanentlyHideable: true
            property int textFormat: Text.AutoText
            function hideClicked() { notificationModel.remove(ObjectModel.index) }
            function closeClicked() { notificationModel.remove(ObjectModel.index) }
            function abortClicked() {}
            function permanentlyHideClicked() { notificationModel.remove(ObjectModel.index) }
            function buttonClicked(index) {}
            function hoverEntered() {}
            function hoverExited() {}
            function linkActivated(link) {}
        }
    }

    onNewFileRequested: console.log("new file")
    onOpenRecentFileRequested: (index) => {
        console.log("open recent file", index)
    }
    onOpenRecoveryFileRequested: (index) => {
        console.log("open recovery file", index)
    }
}
