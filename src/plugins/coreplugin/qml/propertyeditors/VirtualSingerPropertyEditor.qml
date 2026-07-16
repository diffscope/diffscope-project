import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

PropertyEditorGroupBox {
    id: groupBox

    readonly property string sourcesPickerModelMimeType: "application/x.diffscope.sourcespickermodel"

    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper

    readonly property QtObject selectionModel: windowHandle?.projectDocumentContext.document.selectionModel ?? null
    readonly property bool multipleClips: (selectionModel?.selectedCount ?? 0) > 1
    readonly property QtObject singingClip: selectionModel?.clipSelectionModel.selectedItems[0] ?? null
    readonly property bool hasSources: !multipleClips && Boolean(singingClip?.sources)
    readonly property bool architectureMissing: hasSources
                                                && Boolean(clipSingerIdProvider.architectureId)
                                                && !architectureInfoProvider.exists
    readonly property var missingSingerIds: architectureMissing ? [] : singerNameResolver.missingSingerIds
    readonly property bool hasSingerResolutionWarning: architectureMissing || missingSingerIds.length > 0
    readonly property string cardTitle: multipleClips
                                                ? qsTr("Multiple singing clips")
                                                : !hasSources
                                                  ? qsTr("No singer")
                                                  : singerNameResolver.displayName
    readonly property string cardSubtitle: multipleClips
                                           ? ""
                                           : !hasSources
                                             ? qsTr("Tap to set the singer")
                                             : architectureInfoProvider.info.name
                                               || clipSingerIdProvider.architectureId

    title: qsTr("Virtual Singer")
    visualVisible: propertyMapper?.type === 1

    function showSingerResolutionWarning() {
        if (architectureMissing) {
            groupBox.MessageBox.warning(
                qsTr("Singer architecture unavailable"),
                qsTr('The singer architecture "%1" is not registered. Singer information cannot be resolved.')
                    .arg(clipSingerIdProvider.architectureId)
            )
            return
        }
        if (missingSingerIds.length === 0)
            return

        const architectureName = architectureInfoProvider.info.name || qsTr("Unnamed architecture")
        const heading = architectureInfoProvider.exists
                        ? qsTr('The following singers are not registered in architecture "%1":')
                            .arg(architectureName)
                        : qsTr("The following singers could not be found:")
        const singerList = "<ul>"
                           + missingSingerIds.map(singerId => "<li>" + singerId + "</li>").join("")
                           + "</ul>"
        groupBox.MessageBox.warning(
            qsTr("Singer information unavailable"),
            heading + singerList
        )
    }

    function selectedSingingClips() {
        const selectedItems = selectionModel?.clipSelectionModel.selectedItems ?? []
        const clips = []
        for (let index = 0; index < selectedItems.length; ++index)
            clips.push(selectedItems[index])
        return clips
    }

    ClipSingerIdProvider {
        id: clipSingerIdProvider
        sources: groupBox.hasSources ? groupBox.singingClip.sources : null
    }

    ArchitectureInfoProvider {
        id: architectureInfoProvider
        registry: CoreInterface.singerRegistry
        architectureId: clipSingerIdProvider.architectureId
    }

    QtObject {
        id: singerNameResolver

        property int providerRevision: 0
        property bool providerRefreshScheduled: false

        readonly property var singerEntries: flattenSingerTree(clipSingerIdProvider.singerTree, "")
        readonly property string displayName: formatRootSingerTree(clipSingerIdProvider.singerTree)
        property var missingSingerIds: []
        onSingerEntriesChanged: scheduleProviderRefresh()

        readonly property Instantiator singerNameInstantiator: Instantiator {
            model: groupBox.architectureMissing ? [] : singerNameResolver.singerEntries
            onObjectAdded: singerNameResolver.scheduleProviderRefresh()
            onObjectRemoved: singerNameResolver.scheduleProviderRefresh()

            delegate: SingerInfoProvider {
                id: singerInfoProvider

                required property var modelData
                readonly property var entry: modelData
                readonly property string resolvedName: info.name || singerId

                registry: CoreInterface.singerRegistry
                architectureId: clipSingerIdProvider.architectureId
                singerId: entry?.singerId ?? ""

                Component.onCompleted: singerNameResolver.scheduleProviderRefresh()
                onArchitectureIdChanged: singerNameResolver.scheduleProviderRefresh()
                onSingerIdChanged: singerNameResolver.scheduleProviderRefresh()
                onInfoChanged: singerNameResolver.scheduleProviderRefresh()
                onExistsChanged: singerNameResolver.scheduleProviderRefresh()
            }
        }

        function scheduleProviderRefresh() {
            if (providerRefreshScheduled)
                return
            providerRefreshScheduled = true
            missingSingerIds = []
            Qt.callLater(() => {
                providerRefreshScheduled = false
                ++providerRevision
                const resolvedMissingSingerIds = findMissingSingerIds(singerEntries)
                if (!providerRefreshScheduled)
                    missingSingerIds = resolvedMissingSingerIds
            })
        }

        function isSingerGroup(node) {
            return typeof node !== "string"
                    && node !== null
                    && node !== undefined
                    && typeof node.length === "number"
        }

        function flattenSingerTree(tree, parentPath) {
            if (!isSingerGroup(tree))
                return []

            let result = []
            for (let index = 0; index < tree.length; ++index) {
                const node = tree[index]
                const path = parentPath ? parentPath + "/" + index : String(index)
                if (typeof node === "string") {
                    result.push({
                        path: path,
                        singerId: node
                    })
                } else if (isSingerGroup(node)) {
                    result = result.concat(flattenSingerTree(node, path))
                }
            }
            return result
        }

        function resolvedSingerName(path, singerId) {
            providerRevision
            const provider = providerForEntry(path, singerId)
            return provider ? provider.resolvedName : singerId
        }

        function providerForEntry(path, singerId) {
            for (let index = 0; index < singerNameInstantiator.count; ++index) {
                const provider = singerNameInstantiator.objectAt(index)
                const entry = provider?.entry
                if (entry
                        && entry.path === path
                        && entry.singerId === singerId
                        && provider.singerId === singerId
                        && provider.architectureId === clipSingerIdProvider.architectureId) {
                    return provider
                }
            }
            return null
        }

        function findMissingSingerIds(entries) {
            let missingIds = []
            for (let index = 0; index < entries.length; ++index) {
                const entry = entries[index]
                const singerId = entry.singerId
                if (singerId === "")
                    continue
                const provider = providerForEntry(entry.path, singerId)
                if (provider && !provider.exists && !missingIds.includes(singerId))
                    missingIds.push(singerId)
            }
            return missingIds
        }

        function formatSingerNode(node, path) {
            if (typeof node === "string")
                return resolvedSingerName(path, node)
            if (!isSingerGroup(node))
                return ""

            let names = []
            for (let index = 0; index < node.length; ++index) {
                const childPath = path ? path + "/" + index : String(index)
                names.push(formatSingerNode(node[index], childPath))
            }
            return qsTr("Mixed singer (%1)").arg(names.join(qsTr(", ")))
        }

        function formatRootSingerTree(tree) {
            if (!isSingerGroup(tree))
                return ""

            singerNameInstantiator.count
            let names = []
            for (let index = 0; index < tree.length; ++index)
                names.push(formatSingerNode(tree[index], String(index)))
            if (names.length > 1)
                return qsTr("Mixed singer (%1)").arg(names.join(qsTr(", ")))
            return names.length === 1 ? names[0] : ""
        }
    }

    EditSourcesScenario {
        id: editSourcesScenario
        window: groupBox.windowHandle?.window ?? null
        document: groupBox.windowHandle?.projectDocumentContext.document ?? null
    }

    SourcesPickerModel {
        id: sourcesPickerModel
    }

    T.Button {
        id: singerCardButton

        width: parent.width
        implicitHeight: 60
        onClicked: {
            sourcesPickerModel.fromSources(groupBox.singingClip?.sources ?? null)
            editSourcesScenario.editSources(
                sourcesPickerModel,
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
                editSourcesScenario.applySources(
                    sourcesPickerModel,
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

        Card {
            id: singerCard

            anchors.fill: parent
            visible: !singerDropArea.containsDrag
            property color _baseColor: Theme.backgroundColor(groupBox.ThemedItem.backgroundLevel)
            property color color: singerCardButton.pressed
                                  ? Theme.controlPressedColorChange.apply(_baseColor)
                                  : singerCardButton.hovered
                                    ? Theme.controlHoveredColorChange.apply(_baseColor)
                                    : _baseColor

            Behavior on color {
                ColorAnimation {
                    duration: Theme.colorAnimationDuration
                    easing.type: Easing.OutCubic
                }
            }

            onColorChanged: background.color = color

            image: Item {
                SingerAvatar {
                    anchors.fill: parent
                    visible: groupBox.hasSources
                    architectureId: clipSingerIdProvider.architectureId
                    singerTree: clipSingerIdProvider.singerTree
                }

                Rectangle {
                    anchors.fill: parent
                    visible: !groupBox.hasSources
                    color: Theme.backgroundQuaternaryColor

                    IconLabel {
                        anchors.centerIn: parent
                        icon.source: `image://fluent-system-icons/${groupBox.multipleClips ? "mic" : "mic_off"}?size=32&style=regular`
                        icon.color: Theme.foregroundSecondaryColor
                        icon.width: 32
                        icon.height: 32
                    }
                }
            }

            title: Label {
                text: groupBox.cardTitle
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            subtitle: Label {
                text: groupBox.cardSubtitle
                visible: Boolean(text)
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            toolBar: ToolButton {
                visible: groupBox.hasSingerResolutionWarning
                flat: true
                display: AbstractButton.IconOnly
                text: qsTr("Singer information warning")
                icon.source: "image://fluent-system-icons/warning"
                icon.color: Theme.warningColor
                icon.width: 20
                icon.height: 20
                ToolTip.visible: hovered
                ToolTip.text: text
                onClicked: groupBox.showSingerResolutionWarning()
            }
        }
    }
}
