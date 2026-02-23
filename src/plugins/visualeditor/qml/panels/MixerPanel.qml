import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

import QActionKit

QtObject {
	id: d
	required property QtObject addOn
    required property QtObject scrollAddOn

	readonly property Component mixerPanelComponent: ActionDockingPane {
		function loadState(state) {
			if (!state)
				return
			d.addOn.mixerPanelInterface.tool = state.tool
		}

		function saveState() {
			return {
				tool: d.addOn?.mixerPanelInterface.tool,
			}
		}

        Docking.onActivated: d.scrollAddOn.activeEditingArea = 2

		header: ToolBarContainer {
			anchors.fill: parent
			property MenuActionInstantiator instantiator: MenuActionInstantiator {
				actionId: "org.diffscope.visualeditor.mixerPanelToolBar"
				context: d.addOn?.windowHandle.actionContext ?? null
				separatorComponent: ToolBarContainerSeparator {
				}
				stretchComponent: ToolBarContainerStretch {
				}
				Component.onCompleted: forceUpdateLayouts()
			}
		}

		data: [d.addOn?.mixerPanelInterface.mixerView ?? null]
	}
}