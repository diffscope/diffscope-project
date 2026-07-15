import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

Item {
    id: control

    required property SourcesPickerModel sourcesModel
    required property var modelIndex

    readonly property bool modelIndexValid: {
        sourcesModel.revision
        return Boolean(modelIndex && sourcesModel.indexAlive(modelIndex))
    }
    readonly property string singerId: modelIndexValid ? sourcesModel.singerId(modelIndex) : ""
    readonly property bool singerValid: modelIndexValid && sourcesModel.singerValid(modelIndex)
    readonly property bool canLoadPanel: architectureInfoProvider.exists
                                         && singerInfoProvider.exists
                                         && Boolean(architectureInfoProvider.info.controlPanelComponent)

    onModelIndexChanged: Qt.callLater(initializePanel)
    onSingerIdChanged: Qt.callLater(initializePanel)

    ArchitectureInfoProvider {
        id: architectureInfoProvider
        registry: control.sourcesModel.registry
        architectureId: control.sourcesModel.architectureId
    }

    SingerInfoProvider {
        id: singerInfoProvider
        registry: control.sourcesModel.registry
        architectureId: control.sourcesModel.architectureId
        singerId: control.singerId
    }

    Loader {
        id: panelLoader
        anchors.fill: parent
        active: control.canLoadPanel
        sourceComponent: active ? architectureInfoProvider.info.controlPanelComponent : null

        onLoaded: control.initializePanel()
    }

    Connections {
        target: panelLoader.item
        ignoreUnknownSignals: true

        function onExtraEdited() {
            if (panelLoader.item && typeof panelLoader.item.getExtra === "function")
                control.sourcesModel.setSingerExtra(control.modelIndex, panelLoader.item.getExtra())
        }
    }

    Label {
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 420)
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        visible: !panelLoader.active
        ThemedItem.foregroundLevel: SVS.FL_Secondary
        text: {
            if (!architectureInfoProvider.exists)
                return qsTr("The current singer architecture is unavailable.")
            if (!singerInfoProvider.exists)
                return qsTr("The current singer is unavailable.")
            return qsTr("The current singer requires no additional configuration.")
        }
    }

    function initializePanel() {
        if (!modelIndexValid || !panelLoader.item)
            return
        if (typeof panelLoader.item.setSingerId === "function")
            panelLoader.item.setSingerId(singerId)
        if (typeof panelLoader.item.setExtra === "function")
            panelLoader.item.setExtra(sourcesModel.singerExtra(modelIndex))
    }

}
