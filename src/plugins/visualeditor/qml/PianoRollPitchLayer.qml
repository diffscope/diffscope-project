// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views
import dev.sjimo.ScopicFlow.Internal as ScopicFlowInternal

import SVSCraft.UIComponents

Item {
    id: pitchLayer

    readonly property PianoRollPanelInterface pianoRollPanelInterface: ViewLayer.pianoRollPanelInterface
    readonly property ProjectViewModelContext projectViewModelContext: ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null
    readonly property alias editor: pitchEditor
    readonly property alias clipViewModel: pitchProxyTimeViewModel.clipViewModel

    z: 2

    ScopicFlowInternal.ClipMappedProxyTimeViewModel {
        id: pitchProxyTimeViewModel
        timeViewModel: pitchLayer.pianoRollPanelInterface?.timeViewModel ?? null
        clipViewModel: pitchLayer.projectViewModelContext?.getClipViewItemFromDocumentItem(pitchLayer.pianoRollPanelInterface?.editingClip ?? null) ?? null
    }
    Item {
        id: pitchEditorViewport
        width: parent.width
        height: pitchLayer.ViewLayer.visibleRect.height
        clip: true

        ClavierManipulator {
            id: pitchClavierManipulator
            clavierViewModel: pitchLayer.pianoRollPanelInterface?.clavierViewModel ?? null
            target: pitchEditorViewport
        }
        ParameterEditor {
            id: pitchEditor
            verticalManipulator: pitchClavierManipulator
            verticalViewport: pitchEditorViewport
            readonly property ParameterViewModelBinding binding:
                pitchLayer.projectViewModelContext?.parameterEditorContext.pitchBinding ?? null
            readonly property real keyHeight:
                pitchLayer.pianoRollPanelInterface?.clavierViewModel?.pixelDensity ?? 0
            readonly property real timePixelDensity:
                pitchLayer.pianoRollPanelInterface?.timeLayoutViewModel?.pixelDensity ?? 0
            readonly property real clavierStart:
                pitchLayer.pianoRollPanelInterface?.clavierViewModel?.start ?? 0
            readonly property real key128CenterY:
                (clavierStart - 128.5) * keyHeight
            readonly property real key0CenterY:
                (clavierStart - 0.5) * keyHeight
            readonly property bool noteEditPitchCurveVisible: {
                if (EditorPreference.noteEditPitchCurveDisplayMode === EditorPreference.PCDM_AlwaysShow)
                    return true
                if (EditorPreference.noteEditPitchCurveDisplayMode === EditorPreference.PCDM_AlwaysHide)
                    return false
                return timePixelDensity >= EditorPreference.autoPitchCurveDisplayPixelDensityThreshold
            }
            readonly property real noteEditPitchCurveOpacityFactor: {
                if (EditorPreference.noteEditPitchCurveDisplayMode !== EditorPreference.PCDM_Automatic)
                    return 1.0
                const ratio = timePixelDensity / EditorPreference.autoPitchCurveDisplayPixelDensityThreshold
                return Math.max(0.0, Math.min(1.0, 2 * Math.log2(ratio)))
            }
            x: 0
            width: parent.width
            y: key128CenterY
            height: key0CenterY - key128CenterY
            visible: (binding?.available ?? false) && (enabled || noteEditPitchCurveVisible)
            enabled: pitchLayer.pianoRollPanelInterface?.pitchToolActive ?? false
            opacity: enabled ? 1.0 : 0.5 * noteEditPitchCurveOpacityFactor
            freeParameterViewModel: binding?.freeEdited ?? null
            anchorParameterViewModel: binding?.anchorEdited ?? null
            originalParameterViewModel: binding?.original ?? null
            freeTransformParameterViewModel: binding?.freeTransform ?? null
            anchorTransformParameterViewModel: binding?.anchorTransform ?? null
            freeParameterSelectionViewModel: binding?.freeSelection ?? null
            anchorSelectionController: binding?.anchorSelectionController ?? null
            interactionController: binding?.interactionController ?? null
            timeViewModel: pitchProxyTimeViewModel
            timeLayoutViewModel: pitchLayer.pianoRollPanelInterface?.timeLayoutViewModel ?? null
            Theme.accentColor: Theme.foregroundPrimaryColor
        }
    }
}
