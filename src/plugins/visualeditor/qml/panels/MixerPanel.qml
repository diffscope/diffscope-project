// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.VisualEditor

QtObject {
    id: d
    required property QtObject addOn
    required property QtObject scrollAddOn

    readonly property Component mixerPanelComponent: ActionDockingPane {
        preferredSize: Qt.size(1152, 480)

        function loadState(state) {
            d.addOn.mixerPanelInterface.tool = state?.tool === MixerPanelInterface.HandTool
                ? MixerPanelInterface.HandTool : MixerPanelInterface.PointerTool
        }

        function saveState() {
            return {tool: d.addOn.mixerPanelInterface.tool}
        }

        Docking.onActivated: d.scrollAddOn.activeEditingArea = 2

        header: ToolBarContainer {
            anchors.fill: parent

            property MenuActionInstantiator instantiator: MenuActionInstantiator {
                actionId: "org.diffscope.visualeditor.mixerPanelToolBar"
                context: d.addOn?.windowHandle.actionContext ?? null
                separatorComponent: ToolBarContainerSeparator {}
                stretchComponent: ToolBarContainerStretch {}
                Component.onCompleted: forceUpdateLayouts()
            }
        }

        data: [d.addOn?.mixerPanelInterface.mixerView ?? null]
    }
}
