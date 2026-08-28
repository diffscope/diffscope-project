// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QActionKit

import SVSCraft

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn

    readonly property ProjectViewModelContext projectViewModelContext: addOn?.windowHandle.ProjectViewModelContext.context ?? null

    readonly property Component clipTrackComponent: RangeIndicatorSequence {
        id: control

        required property QtObject contextObject

        readonly property var editingTrackViewModel:
            d.projectViewModelContext?.getTrackViewItemFromDocumentItem(
                contextObject?.editingClip?.clipSequence?.track ?? null) ?? null

        Layout.fillWidth: true
        Theme.accentColor: editingTrackViewModel?.color ?? Qt.rgba(0, 0, 0, 0)
        rangeIndicatorSequenceViewModel:
            d.projectViewModelContext?.getSingingClipRangeIndicatorSequenceViewModel(
                contextObject?.editingClip?.clipSequence?.track ?? null) ?? null
        rangeIndicatorInteractionController:
            contextObject?.clipRangeIndicatorInteractionController ?? null
        scrollBehaviorViewModel: contextObject?.scrollBehaviorViewModel ?? null
        timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
        timeViewModel: contextObject?.timeViewModel ?? null

        Menu {
            id: clipItemContextMenu

            MenuActionInstantiator {
                actionId: "org.diffscope.core.clipItemContextMenu"
                context: d.addOn?.windowHandle.actionContext ?? null
                Component.onCompleted: forceUpdateLayouts()
            }
        }

        Connections {
            target: control.rangeIndicatorInteractionController

            function onItemContextMenuRequested(rangeIndicatorSequence) {
                if (rangeIndicatorSequence === control)
                    clipItemContextMenu.popup()
            }
        }
    }

}
