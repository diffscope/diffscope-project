// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: root

    required property QtObject server
    required property ActionWindowInterfaceBase windowHandle

    ActionItem {
        actionId: "org.diffscope.synth.dssp.statusMenuItem"
        MenuItem {
            enabled: false
            text: root.server.statusMenuText
        }
    }

    ActionItem {
        actionId: "org.diffscope.synth.dssp.start"
        Action {
            enabled: root.server.canStart
            onTriggered: root.server.start()
        }
    }

    ActionItem {
        actionId: "org.diffscope.synth.dssp.stop"
        Action {
            enabled: root.server.canStop
            onTriggered: root.server.stop()
        }
    }

    ActionItem {
        actionId: "org.diffscope.synth.dssp.restart"
        Action {
            enabled: root.server.canRestart
            onTriggered: root.server.restart()
        }
    }

    ActionItem {
        actionId: "org.diffscope.synth.dssp.settings"
        Action {
            onTriggered: Qt.callLater(() => CoreInterface.execSettingsDialog("org.diffscope.synth.DsspServer", root.windowHandle.window))
        }
    }

    ActionItem {
        actionId: "org.diffscope.synth.dssp.status"
        ToolButton {
            id: statusButton
            text: root.server.statusText
            icon.source: "image://fluent-system-icons/" + root.server.statusIcon
            icon.color: {
                if (root.server.statusIcon === "dssp_error")
                    return Theme.errorColor
                if (root.server.statusIcon === "dssp")
                    return Theme.foregroundPrimaryColor
                return Theme.foregroundSecondaryColor
            }
            display: AbstractButton.IconOnly
            Accessible.description: qsTr("Show DiffSinger Synthesis Program controls")
            onClicked: serverMenu.popup(statusButton, mirrored ? width - serverMenu.width : 0, -serverMenu.implicitHeight)

            Menu {
                id: serverMenu
                MenuActionInstantiator {
                    actionId: "org.diffscope.synth.dssp"
                    context: root.windowHandle.actionContext
                    Component.onCompleted: forceUpdateLayouts()
                }
            }
        }
    }
}
