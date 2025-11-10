import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views

Item {
    id: view

    required property ArrangementPanelInterface arrangementPanelInterface
    required property ProjectViewModelContext projectViewModelContext

    anchors.fill: parent

    SplitView {
        id: splitView
        anchors.fill: parent
        orientation: Qt.Horizontal
        Item {
            // TODO
            SplitView.preferredWidth: 200
        }
        Item {
            SplitView.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Timeline {
                    id: timeline
                    Layout.fillWidth: true
                    timeViewModel: view.arrangementPanelInterface?.timeViewModel ?? null
                    timeLayoutViewModel: view.arrangementPanelInterface?.timeLayoutViewModel ?? null
                    playbackViewModel: view.projectViewModelContext?.playbackViewModel ?? null
                    scrollBehaviorViewModel: view.arrangementPanelInterface?.scrollBehaviorViewModel ?? null
                    timelineInteractionController: view.arrangementPanelInterface?.timelineInteractionController ?? null
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
    }

}