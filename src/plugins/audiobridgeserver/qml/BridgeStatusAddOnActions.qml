// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QActionKit

import SVSCraft.Extras
import SVSCraft.UIComponents

import DiffScope.AudioBridgeServer
import DiffScope.Core

ActionCollection {
    id: root

    required property BridgeStatusAddOn addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null

    ActionItem {
        actionId: "org.diffscope.audiobridgeserver.managedProjectStatus"

        Control {
            id: indicator

            readonly property color foregroundColor: ColorUtils.selectHighestContrastColor(Theme.accentColor, [
                Theme.foregroundPrimaryColor, Theme.backgroundPrimaryColor, Theme.navigationColor
            ])

            visible: root.windowHandle?.managed ?? false
            Layout.topMargin: 2
            Layout.bottomMargin: 2
            horizontalPadding: 6
            verticalPadding: 2
            implicitWidth: implicitContentWidth + leftPadding + rightPadding
            implicitHeight: implicitContentHeight + topPadding + bottomPadding

            background: Rectangle {
                color: Theme.accentColor
                radius: 4
            }

            contentItem: IconLabel {
                text: indicator.ActionInstantiator.text
                icon.source: indicator.ActionInstantiator.icon.source
                icon.color: indicator.foregroundColor
                color: indicator.foregroundColor
                mirrored: indicator.mirrored
                Accessible.name: text
            }
        }
    }
}
