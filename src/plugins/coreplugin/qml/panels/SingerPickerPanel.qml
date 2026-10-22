// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

ActionDockingPane {
    id: pane
    preferredSize: Qt.size(400, 480)

    signal singerSelected(string architectureId, string singerId)

    header: Item {
        anchors.fill: parent
        ToolButton {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            icon.source: "image://fluent-system-icons/arrow_sync"
            text: qsTr("Refresh")
            onClicked: () => {
                CoreInterface.singerRegistry.refreshRequested()
            }
        }
    }

    SingerPicker {
        anchors.fill: parent
        draggable: true
        onSingerSelected: (architectureId, singerId) => {
            pane.singerSelected(architectureId, singerId)
        }
    }
}
