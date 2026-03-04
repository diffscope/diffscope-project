import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

import QActionKit

ActionCollection {
    id: d

    required property QtObject addOn

    ActionItem {
        actionId: "org.diffscope.core.notificationErrorIndicator"
        Row {
            id: indicator
            spacing: 4
            property bool activated: false
            Accessible.role: Accessible.StaticText
            Accessible.name: qsTr("%Ln error(s)", "", d.addOn?.notificationManager.criticalCount ?? 0) + " " + qsTr("%Ln warning(s)", "", d.addOn?.notificationManager.warningCount ?? 0)
            IconLabel {
                icon.source: "image://fluent-system-icons/error_circle"
                icon.width: 14
                icon.height: 14
                leftPadding: 4
                icon.color: indicator.activated ? Theme.errorColor : Theme.foregroundPrimaryColor
                text: (d.addOn?.notificationManager.criticalCount ?? 0).toLocaleString()
                color: indicator.activated ? Theme.errorColor : Theme.foregroundPrimaryColor
                font: Theme.font
                spacing: 2
                visible: (d.addOn?.notificationManager.criticalCount ?? 0) > 0
            }
            IconLabel {
                icon.source: "image://fluent-system-icons/warning"
                icon.width: 14
                icon.height: 14
                rightPadding: 4
                icon.color: indicator.activated ? Theme.warningColor : Theme.foregroundPrimaryColor
                text: (d.addOn?.notificationManager.warningCount ?? 0).toLocaleString()
                color: indicator.activated ? Theme.warningColor : Theme.foregroundPrimaryColor
                font: Theme.font
                spacing: 2
                visible: (d.addOn?.notificationManager.warningCount ?? 0) > 0
            }
            Connections {
                target: d.addOn?.notificationManager ?? null
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
