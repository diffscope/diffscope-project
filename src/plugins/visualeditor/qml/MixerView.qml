// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

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

    readonly property Item centerEditArea: trackMixer

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
            SplitView.minimumWidth: 128
            SplitView.maximumWidth: contentWidth - 80
            SplitView.preferredWidth: contentWidth - 80
            trackListViewModel: view.projectViewModelContext?.masterTrackListViewModel ?? null
            trackListLayoutViewModel: view.mixerPanelInterface?.masterTrackListLayoutViewModel ?? null
            scrollBehaviorViewModel: view.mixerPanelInterface?.scrollBehaviorViewModel ?? null
            trackListInteractionController: view.mixerPanelInterface?.masterTrackListInteractionController ?? null
            selectionController: null
        }
    }
}
