import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Layouts

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn

    readonly property Component arrangementPanelComponent: ActionDockingPane {

        function loadState(state) {
            if (!state)
                return
            let arrangementPanelInterface = addOn.arrangementPanelInterface
            arrangementPanelInterface.tool = state.tool
            arrangementPanelInterface.positionAlignmentManipulator.duration = state.duration
            arrangementPanelInterface.positionAlignmentManipulator.tuplet = state.tuplet
            arrangementPanelInterface.autoPageScrollingManipulator.enabled = state.isAutoPageScrollingEnabled
            for (let id of state.additionalTracks) {
                d.addOn.additionalTrackLoader.loadItem(id)
            }
        }

        function saveState() {
            let arrangementPanelInterface = addOn.arrangementPanelInterface
            return {
                tool: arrangementPanelInterface.tool,
                duration: arrangementPanelInterface.positionAlignmentManipulator.duration,
                tuplet: arrangementPanelInterface.positionAlignmentManipulator.tuplet,
                isAutoPageScrollingEnabled: arrangementPanelInterface.autoPageScrollingManipulator.enabled,
                additionalTracks: d.addOn.additionalTrackLoader.loadedComponents
            }
        }

        Component {
            id: dummyItem
            Item {}
        }
        header: ToolBarContainer {
            id: toolBar
            anchors.fill: parent
            property MenuActionInstantiator instantiator: MenuActionInstantiator {
                actionId: "org.diffscope.visualeditor.arrangementPanelToolBar"
                context: d.addOn?.windowHandle.actionContext ?? null
                separatorComponent: ToolBarContainerSeparator {
                }
                stretchComponent: ToolBarContainerStretch {
                }
                Component.onCompleted: forceUpdateLayouts()
            }
        }
        data: [d.addOn?.arrangementPanelInterface.arrangementView ?? null]
    }

}