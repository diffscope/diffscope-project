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

    ActionItem {
        actionId: "org.diffscope.core.notificationErrorIndicator"
        RowLayout {
            id: indicator
            spacing: 4
            property bool activated: true
            Accessible.role: Accessible.StaticText
            Layout.fillWidth: false
            Accessible.name: qsTr("%Ln error(s)", "", d.addOn?.notificationViewModel.criticalCount ?? 0) + " " + qsTr("%Ln warning(s)", "", d.addOn?.notificationViewModel.warningCount ?? 0)
            IconLabel {
                icon.source: "image://fluent-system-icons/error_circle"
                icon.width: 14
                icon.height: 14
                leftPadding: 4
                icon.color: indicator.activated ? Theme.errorColor : Theme.foregroundPrimaryColor
                text: (d.addOn?.notificationViewModel.criticalCount ?? 0).toLocaleString()
                color: indicator.activated ? Theme.errorColor : Theme.foregroundPrimaryColor
                spacing: 2
                visible: (d.addOn?.notificationViewModel.criticalCount ?? 0) > 0
            }
            IconLabel {
                icon.source: "image://fluent-system-icons/warning"
                icon.width: 14
                icon.height: 14
                rightPadding: 4
                icon.color: indicator.activated ? Theme.warningColor : Theme.foregroundPrimaryColor
                text: (d.addOn?.notificationViewModel.warningCount ?? 0).toLocaleString()
                color: indicator.activated ? Theme.warningColor : Theme.foregroundPrimaryColor
                spacing: 2
                visible: (d.addOn?.notificationViewModel.warningCount ?? 0) > 0
            }
            Connections {
                target: d.addOn?.notificationViewModel ?? null
                function onErrorActivated() {
                    indicator.activated = true
                }
            }
            Connections {
                target: d.addOn
                function onDeactivateIndicator() {
                    indicator.activated = false
                }
            }
            TapHandler {
                onSingleTapped: d.addOn.showPanelRequested()
            }
        }
    }

}
