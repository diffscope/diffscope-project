// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QActionKit

import SVSCraft.UIComponents

ActionCollection {
    id: d

    required property QtObject addOn

    readonly property int errorCount: addOn?.documentProblems.errorCount ?? 0
    readonly property int warningCount: addOn?.documentProblems.warningCount ?? 0

    ActionItem {
        actionId: "org.diffscope.core.problemIndicator"
        RowLayout {
            spacing: 4
            Accessible.role: Accessible.StaticText
            Layout.fillWidth: false
            Accessible.name: qsTr("%Ln error(s)", "", d.errorCount) + " " + qsTr("%Ln warning(s)", "", d.warningCount)
            IconLabel {
                icon.source: "image://fluent-system-icons/error_circle"
                icon.width: 14
                icon.height: 14
                leftPadding: 4
                icon.color: Theme.errorColor
                text: d.errorCount.toLocaleString()
                color: Theme.errorColor
                spacing: 2
                visible: d.errorCount > 0
            }
            IconLabel {
                icon.source: "image://fluent-system-icons/warning"
                icon.width: 14
                icon.height: 14
                rightPadding: 4
                icon.color: Theme.warningColor
                text: d.warningCount.toLocaleString()
                color: Theme.warningColor
                spacing: 2
                visible: d.warningCount > 0
            }
            TapHandler {
                onSingleTapped: d.addOn.showPanelRequested()
            }
        }
    }

}
