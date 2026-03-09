import QtQml
import QtQml.Models
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
    required property QtObject scrollAddOn

	readonly property Component pianoRollPanelComponent: ActionDockingPane {
        id: pane

		function loadState(state) {
			if (!state)
				return
			let pianoRollPanelInterface = addOn.pianoRollPanelInterface
            if (state.tool !== undefined)
			    pianoRollPanelInterface.tool = state.tool
            if (state.duration !== undefined)
			    pianoRollPanelInterface.positionAlignmentManipulator.duration = state.duration
            if (state.tuplet !== undefined)
			    pianoRollPanelInterface.positionAlignmentManipulator.tuplet = state.tuplet
            if (state.isAutoPageScrollingEnabled !== undefined)
			    pianoRollPanelInterface.autoPageScrollingManipulator.enabled = state.isAutoPageScrollingEnabled
            if (state.scaleHighlightEnabled !== undefined)
			    pianoRollPanelInterface.scaleHighlightEnabled = state.scaleHighlightEnabled
			for (let id of state.additionalTracks ?? []) {
				d.addOn.additionalTrackLoader.loadItem(id)
			}
            for (let id of state.bottomAdditionalTracks ?? []) {
				d.addOn.bottomAdditionalTrackLoader.loadItem(id)
			}
            if (state.noteAreaSplitViewState !== undefined)
                pianoRollPanelInterface.pianoRollView.noteAreaSplitView.restoreState(state.noteAreaSplitViewState)
		}

		function saveState() {
			let pianoRollPanelInterface = addOn.pianoRollPanelInterface
			return {
				tool: pianoRollPanelInterface.tool,
				duration: pianoRollPanelInterface.positionAlignmentManipulator.duration,
				tuplet: pianoRollPanelInterface.positionAlignmentManipulator.tuplet,
				isAutoPageScrollingEnabled: pianoRollPanelInterface.autoPageScrollingManipulator.enabled,
                scaleHighlightEnabled: pianoRollPanelInterface.scaleHighlightEnabled,
				additionalTracks: d.addOn.additionalTrackLoader.loadedComponents,
                bottomAdditionalTracks: d.addOn.bottomAdditionalTrackLoader.loadedComponents,
                noteAreaSplitViewState: pianoRollPanelInterface.pianoRollView.noteAreaSplitView.saveState()
			}
		}

		Component {
			id: dummyItem
			Item {}
		}

        Docking.onActivated: d.scrollAddOn.activeEditingArea = 1

        Connections {
            target: d.addOn?.pianoRollPanelInterface ?? null
            function onEditingClipChanged() {
                pane.Docking.dockingView.showPane(pane)
            }
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