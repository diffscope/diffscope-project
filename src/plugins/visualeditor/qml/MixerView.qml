import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Internal

Item {
    id: view

    required property QtObject addOn
    required property MixerPanelInterface mixerPanelInterface

    readonly property ProjectViewModelContext projectViewModelContext: addOn?.windowHandle.ProjectViewModelContext.context ?? null

    anchors.fill: parent

    SplitView {
        id: splitView
        anchors.fill: parent
        orientation: Qt.Horizontal

        Mixer {
            id: trackMixer
            SplitView.fillWidth: true
            trackListViewModel: view.projectViewModelContext?.trackListViewModel ?? null
            trackListLayoutViewModel: view.mixerPanelInterface?.trackListLayoutViewModel ?? null
            scrollBehaviorViewModel: view.mixerPanelInterface?.scrollBehaviorViewModel ?? null
            trackListInteractionController: view.mixerPanelInterface?.trackListInteractionController ?? null
            selectionController: view.projectViewModelContext?.trackSelectionController ?? null
        }

        Mixer {
            id: masterMixer
            // TODO There might be more bus tracks in the future
            SplitView.minimumWidth: 128
            SplitView.maximumWidth: 128
            trackListViewModel: view.projectViewModelContext?.masterTrackListViewModel ?? null
            trackListLayoutViewModel: view.mixerPanelInterface?.masterTrackListLayoutViewModel ?? null
            scrollBehaviorViewModel: view.mixerPanelInterface?.scrollBehaviorViewModel ?? null
            trackListInteractionController: view.mixerPanelInterface?.masterTrackListInteractionController ?? null
            selectionController: null
        }
    }
}
