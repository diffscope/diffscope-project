import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

Rectangle {
    id: control

    property string architectureId
    property string mixGroup
    property bool draggable: false

    readonly property string sourcesPickerModelMimeType: "application/x.diffscope.sourcespickermodel"

    readonly property var filteredArchitectureIds: CoreInterface.singerRegistry.architectureIds.filter(
                                                           id => control.architectureId === ""
                                                                 || id === control.architectureId
                                                       )

    signal singerSelected(string architectureId, string singerId)

    color: Theme.backgroundColor(control.ThemedItem.backgroundLevel)

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 8
            spacing: 8

            TextField {
                id: searchTextField

                Layout.fillWidth: true
                placeholderText: qsTr("Search")
                ThemedItem.icon.source: "image://fluent-system-icons/search"
            }
        }

        ScrollView {
            id: singerScrollView

            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            ColumnLayout {
                width: singerScrollView.width
                spacing: 8

                Repeater {
                    model: control.filteredArchitectureIds

                    delegate: GroupBox {
                        id: architectureGroupBox

                        required property var modelData
                        readonly property string architectureId: modelData
                        readonly property string architectureName: architectureInfoProvider.info.name
                                                                   || architectureId
                        readonly property var filteredSingerProviders: filterSingerProviders(
                                                                           architectureInfoProvider.singerIds,
                                                                           searchTextField.text
                                                                       )
                        readonly property Instantiator singerInfoInstantiator: Instantiator {
                            model: architectureInfoProvider.singerIds

                            delegate: SingerInfoProvider {
                                required property var modelData
                                readonly property string entrySingerId: modelData

                                registry: CoreInterface.singerRegistry
                                architectureId: architectureGroupBox.architectureId
                                singerId: entrySingerId
                            }
                        }

                        Layout.fillWidth: true
                        leftPadding: 8
                        rightPadding: 8
                        title: architectureName
                        visible: filteredSingerProviders.length > 0

                        function includesSearchText(text, normalizedSearchText) {
                            return String(text).toLocaleLowerCase().indexOf(normalizedSearchText) !== -1
                        }

                        function filterSingerProviders(singerIds, searchText) {
                            singerIds.length
                            singerInfoInstantiator.count

                            const normalizedSearchText = searchText.trim().toLocaleLowerCase()
                            const architectureMatches = normalizedSearchText.length > 0
                                                        && (includesSearchText(architectureId, normalizedSearchText)
                                                            || includesSearchText(architectureName, normalizedSearchText))
                            let result = []
                            for (let index = 0; index < singerInfoInstantiator.count; ++index) {
                                const provider = singerInfoInstantiator.objectAt(index)
                                if (!provider)
                                    continue
                                if (control.mixGroup !== ""
                                        && provider.info.mixGroup !== control.mixGroup) {
                                    continue
                                }

                                const singerName = provider.info.name || provider.entrySingerId
                                if (normalizedSearchText.length === 0
                                        || architectureMatches
                                        || includesSearchText(provider.entrySingerId, normalizedSearchText)
                                        || includesSearchText(singerName, normalizedSearchText)) {
                                    result.push(provider)
                                }
                            }
                            return result
                        }

                        ArchitectureInfoProvider {
                            id: architectureInfoProvider
                            registry: CoreInterface.singerRegistry
                            architectureId: architectureGroupBox.architectureId
                        }

                        ColumnLayout {
                            width: architectureGroupBox.width
                                   - architectureGroupBox.leftPadding
                                   - architectureGroupBox.rightPadding
                            spacing: 0

                            Repeater {
                                id: singerRepeater
                                model: architectureGroupBox.filteredSingerProviders

                                delegate: T.Button {
                                    id: singerButton

                                    required property int index
                                    required property var modelData
                                    readonly property SingerInfoProvider singerInfoProvider: modelData
                                    property bool dragPayloadReady: false

                                    Layout.fillWidth: true
                                    height: 60
                                    Accessible.role: Accessible.ListItem
                                    Accessible.name: singerInfoProvider.info.name
                                                     || singerInfoProvider.entrySingerId
                                    Drag.keys: [control.sourcesPickerModelMimeType]
                                    Drag.mimeData: ({})
                                    Drag.supportedActions: Qt.CopyAction
                                    Drag.proposedAction: Qt.CopyAction
                                    Drag.dragType: Drag.Automatic
                                    Drag.active: control.draggable
                                                 && singerDragHandler.active
                                                 && dragPayloadReady
                                    Drag.hotSpot.x: width / 2
                                    Drag.hotSpot.y: height / 2
                                    onClicked: control.singerSelected(
                                                   architectureGroupBox.architectureId,
                                                   singerInfoProvider.entrySingerId
                                               )

                                    SourcesPickerModel {
                                        id: dragSourcesModel

                                        architectureId: architectureGroupBox.architectureId
                                    }

                                    DragHandler {
                                        id: singerDragHandler

                                        enabled: control.draggable
                                        target: null
                                        onActiveChanged: {
                                            singerButton.dragPayloadReady = false
                                            if (!active)
                                                return

                                            dragSourcesModel.fromDefaultSinger(
                                                        singerButton.singerInfoProvider.entrySingerId)
                                            if (dragSourcesModel.empty || !dragSourcesModel.valid)
                                                return

                                            const mimeData = {}
                                            mimeData[control.sourcesPickerModelMimeType]
                                                    = dragSourcesModel.serialize()
                                            singerButton.Drag.mimeData = mimeData
                                            singerButton.dragPayloadReady = true
                                        }
                                    }

                                    Card {
                                        anchors.fill: parent
                                        atTop: singerButton.index === 0
                                        atBottom: singerButton.index === singerRepeater.count - 1
                                        property color _baseColor: Theme.backgroundColor(
                                                                       control.ThemedItem.backgroundLevel
                                                                   )
                                        property color color: singerButton.pressed
                                                              ? Theme.controlPressedColorChange.apply(_baseColor)
                                                              : singerButton.hovered
                                                                ? Theme.controlHoveredColorChange.apply(_baseColor)
                                                                : _baseColor

                                        Behavior on color {
                                            ColorAnimation {
                                                duration: Theme.colorAnimationDuration
                                                easing.type: Easing.OutCubic
                                            }
                                        }

                                        onColorChanged: background.color = color
                                        title: singerButton.singerInfoProvider.info.name
                                               || singerButton.singerInfoProvider.entrySingerId
                                        subtitle: control.draggable && singerButton.hovered
                                                  ? qsTr("Drag to set singer")
                                                  : architectureGroupBox.architectureName
                                        image: SingerAvatar {
                                            architectureId: architectureGroupBox.architectureId
                                            singerTree: [singerButton.singerInfoProvider.entrySingerId]
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
