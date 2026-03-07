import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Layouts

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn

    readonly property ProjectViewModelContext projectViewModelContext: addOn?.windowHandle.ProjectViewModelContext.context ?? null

    readonly property Component clipTrackComponent: ClipSequence {
        id: control

        required property QtObject contextObject
        Layout.fillWidth: true
        clipSequenceViewModel: d.projectViewModelContext?.getSingingClipPerTrackSequenceViewModel(contextObject.editingClip?.clipSequence?.track ?? null) ?? null
        trackListViewModel: d.projectViewModelContext?.trackListViewModel ?? null
        scrollBehaviorViewModel: contextObject?.scrollBehaviorViewModel ?? null
        timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
        timeViewModel: contextObject?.timeViewModel ?? null
    }

}