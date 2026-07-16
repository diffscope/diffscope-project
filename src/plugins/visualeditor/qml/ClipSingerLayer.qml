import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow

import DiffScope.DspxModel as DspxModel
import DiffScope.Core

Item {
    id: d

    readonly property string sourcesPickerModelMimeType: "application/x.diffscope.sourcespickermodel"

    required property ClipViewModel clipViewModel
    required property bool visualVisible
    required property ProjectViewModelContext projectViewModelContext
    required property double viewportOffset

    readonly property QtObject documentClip: projectViewModelContext?.getClipDocumentItemFromViewItem(clipViewModel) ?? null
    readonly property QtObject singingClip: documentClip?.type === DspxModel.Clip.Singing ? documentClip : null
    readonly property var singerIds: flattenSingerIds(clipSingerIdProvider.singerTree)
    readonly property string singerName: {
        if (!singingClip?.sources || singerIds.length === 0)
            return qsTr("No singer")
        if (singerIds.length > 1)
            return qsTr("Mixed singer")
        return singerInfoProvider.info.name || singerIds[0]
    }

    visible: singingClip !== null
    clip: true

    function flattenSingerIds(tree) {
        if (!tree || typeof tree.length !== "number")
            return []

        let result = []
        for (let index = 0; index < tree.length; ++index) {
            const singer = tree[index]
            if (typeof singer === "string") {
                if (singer !== "")
                    result.push(singer)
            } else {
                result = result.concat(flattenSingerIds(singer))
            }
        }
        return result
    }

    ClipSingerIdProvider {
        id: clipSingerIdProvider
        sources: d.singingClip?.sources ?? null
    }

    SingerInfoProvider {
        id: singerInfoProvider
        registry: CoreInterface.singerRegistry
        architectureId: clipSingerIdProvider.architectureId
        singerId: d.singerIds.length === 1 ? d.singerIds[0] : ""
    }

    SourcesPickerModel {
        id: sourcesPickerModel
    }

    EditSourcesScenario {
        id: editSourcesScenario
        window: d.projectViewModelContext?.windowHandle?.window ?? null
        document: d.projectViewModelContext?.windowHandle?.projectDocumentContext?.document ?? null
    }

    DropArea {
        id: singerDropArea
        anchors.fill: parent
        keys: [d.sourcesPickerModelMimeType]
        onDropped: drop => {
            if (!d.singingClip) {
                drop.accepted = false
                return
            }
            const data = drop.getDataAsArrayBuffer(d.sourcesPickerModelMimeType)
            if (!sourcesPickerModel.deserialize(data)) {
                drop.accepted = false
                return
            }
            editSourcesScenario.applySources(sourcesPickerModel, [d.singingClip])
            drop.acceptProposedAction()
        }
    }

    Rectangle {
        id: dropBackground
        anchors.fill: parent
        visible: singerDropArea.containsDrag
        color: Theme.backgroundQuaternaryColor
        opacity: 0
        SequentialAnimation {
            running: singerDropArea.containsDrag
            loops: Animation.Infinite
            NumberAnimation {
                target: dropBackground
                property: "opacity"
                from: 0
                to: 0.5
                duration: Theme.visualEffectAnimationDuration
                easing.type: Easing.InOutCubic
            }

            NumberAnimation {
                target: dropBackground
                property: "opacity"
                from: 0.5
                to: 0
                duration: Theme.visualEffectAnimationDuration
                easing.type: Easing.InOutCubic
            }
        }
    }

    Label {
        id: singerNameLabel
        x: Math.min(Math.max(8, d.viewportOffset * d.width / (d.clipViewModel?.length ?? 0) + 8), d.width - width - 8)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        color: Theme.foregroundPrimaryColor
        elide: Text.ElideRight
        font: Theme.font
        text: singerDropArea.containsDrag ? qsTr("Drop to set singer") : d.singerName
    }
}
