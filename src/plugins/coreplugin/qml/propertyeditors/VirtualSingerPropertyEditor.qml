// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core
import DiffScope.DspxModel.SelectionModel as DspxSelectionModel
import DiffScope.SingerCatalog
import DiffScope.DspxProject as DspxProject

PropertyEditorGroupBox {
    id: groupBox

    readonly property string sourcesPickerModelMimeType: "application/x.diffscope.sourcespickermodel"

    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper

    readonly property QtObject selectionModel: propertyMapper?.selectionModel ?? null
    readonly property bool multipleClips: selectionModel?.selectionType
                                          === DspxSelectionModel.SelectionModel.ST_Clip
                                          && (selectionModel?.selectedCount ?? 0) > 1
    readonly property QtObject singingClip: {
        switch (selectionModel?.selectionType) {
        case DspxSelectionModel.SelectionModel.ST_Clip:
            return selectionModel.clipSelectionModel.selectedItems[0] ?? null
        case DspxSelectionModel.SelectionModel.ST_Note:
            return selectionModel.noteSelectionModel.noteSequenceWithSelectedItems?.singingClip ?? null
        case DspxSelectionModel.SelectionModel.ST_AnchorNode: {
            const sequence = selectionModel.anchorNodeSelectionModel.anchorNodeSequenceWithSelectedItems
            return sequence?.parameter?.parameterMap?.singingClip ?? null
        }
        case DspxSelectionModel.SelectionModel.ST_DynamicMixingAnchor:
            return selectionModel.dynamicMixingAnchorSelectionModel
                .dynamicMixingAnchorSequenceWithSelectedItems?.sources?.singingClip ?? null
        default:
            return null
        }
    }
    readonly property bool hasSources: !multipleClips && Boolean(singingClip?.sources)

    title: qsTr("Virtual Singer")
    visualVisible: propertyMapper?.kind === 1

    function selectedSingingClips() {
        if (!selectionModel)
            return []

        if (selectionModel.selectionType !== DspxSelectionModel.SelectionModel.ST_Clip)
            return singingClip ? [singingClip] : []

        const selectedItems = selectionModel.clipSelectionModel.selectedItems
        const clips = []
        for (let index = 0; index < selectedItems.length; ++index)
            clips.push(selectedItems[index])
        return clips
    }

    ClipSingerIdProvider {
        id: clipSingerIdProvider
        sources: groupBox.hasSources ? groupBox.singingClip.sources : null
    }

    readonly property DspxProject.EditSourcesScenario editSourcesScenario: windowHandle?.editSourcesScenario ?? null

    SourcesPickerModel {
        id: sourcesPickerModel
    }

    SingerButton {
        width: parent.width
        architectureId: clipSingerIdProvider.architectureId
        singerTree: clipSingerIdProvider.singerTree
        hasSources: groupBox.hasSources
        multipleClips: groupBox.multipleClips
        contentItem.visible: !singerDropArea.containsDrag
        onClicked: {
            sourcesPickerModel.fromSources(groupBox.singingClip?.sources ?? null)
            groupBox.editSourcesScenario?.editSources(
                groupBox.windowHandle.window,
                sourcesPickerModel.sourcesSelection(),
                groupBox.selectedSingingClips()
            )
        }

        DropArea {
            id: singerDropArea

            anchors.fill: parent
            z: 2
            keys: [groupBox.sourcesPickerModelMimeType]
            onDropped: drop => {
                const clips = groupBox.selectedSingingClips()
                if (clips.length === 0) {
                    drop.accepted = false
                    return
                }
                const data = drop.getDataAsArrayBuffer(groupBox.sourcesPickerModelMimeType)
                if (!sourcesPickerModel.deserialize(data)) {
                    drop.accepted = false
                    return
                }
                groupBox.editSourcesScenario?.applySources(
                    groupBox.windowHandle.window,
                    sourcesPickerModel.sourcesSelection(),
                    clips
                )
                drop.acceptProposedAction()
            }
        }

        Label {
            anchors.centerIn: parent
            z: 1
            visible: singerDropArea.containsDrag
            text: qsTr("Drop to set singer")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
