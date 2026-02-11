import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
	id: d
	required property QtObject addOn

	readonly property Component pianoRollPanelComponent: ActionDockingPane {

		function loadState(state) {
			if (!state)
				return
			let pianoRollPanelInterface = addOn.pianoRollPanelInterface
			pianoRollPanelInterface.tool = state.tool
			pianoRollPanelInterface.positionAlignmentManipulator.duration = state.duration
			pianoRollPanelInterface.positionAlignmentManipulator.tuplet = state.tuplet
			pianoRollPanelInterface.autoPageScrollingManipulator.enabled = state.isAutoPageScrollingEnabled
		}

		function saveState() {
			let pianoRollPanelInterface = addOn.pianoRollPanelInterface
			return {
				tool: pianoRollPanelInterface.tool,
				duration: pianoRollPanelInterface.positionAlignmentManipulator.duration,
				tuplet: pianoRollPanelInterface.positionAlignmentManipulator.tuplet,
				isAutoPageScrollingEnabled: pianoRollPanelInterface.autoPageScrollingManipulator.enabled,
			}
		}

		Component {
			id: dummyItem
			Item {}
		}
		header: ToolBarContainer {
			anchors.fill: parent
			property MenuActionInstantiator instantiator: MenuActionInstantiator {
				actionId: "org.diffscope.visualeditor.pianoRollPanelToolBar"
				context: d.addOn?.windowHandle.actionContext ?? null
				separatorComponent: ToolBarContainerSeparator {
				}
				stretchComponent: ToolBarContainerStretch {
				}
				Component.onCompleted: forceUpdateLayouts()
			}
		}
		data: [d.addOn?.pianoRollPanelInterface.pianoRollView ?? null]
	}

}