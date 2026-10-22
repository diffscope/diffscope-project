// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

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
    readonly property var presetEntries: CoreInterface.mixedSingerPresets.presets

    signal singerSelected(SourcesPickerModel sources)

    color: Theme.backgroundColor(control.ThemedItem.backgroundLevel)

    MixedSingerPresetNameDialog {
        id: presetNameDialog
        anchors.centerIn: Overlay.overlay
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        TabBar {
            id: singerTypeTabBar
            Layout.fillWidth: true
            ThemedItem.flat: true

            TabButton {
                text: qsTr("Single Singers")
                ThemedItem.tabIndicator: SVS.TI_Bottom
            }

            TabButton {
                text: qsTr("Mixed Singers")
                ThemedItem.tabIndicator: SVS.TI_Bottom
            }
        }

        TextField {
            id: searchTextField
            Layout.fillWidth: true
            Layout.margins: 8
            placeholderText: qsTr("Search")
            Accessible.name: placeholderText
            ThemedItem.icon.source: "image://fluent-system-icons/search"
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: singerTypeTabBar.currentIndex

            Repeater {
                model: 2

                delegate: ScrollView {
                    id: singerScrollView

                    required property int index
                    readonly property bool mixedPage: index === 1
                    readonly property var filteredArchitectureIds: {
                        const ids = mixedPage
                                    ? [...new Set(control.presetEntries.map(entry => entry.sourcesModel.architectureId))].sort()
                                    : CoreInterface.singerRegistry.architectureIds
                        return ids.filter(id => control.architectureId === "" || id === control.architectureId)
                    }

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: singerScrollView.availableWidth
                        spacing: 8

                        Repeater {
                            model: singerScrollView.filteredArchitectureIds

                            delegate: GroupBox {
                                id: architectureGroupBox

                                required property var modelData
                                property int providerRevision: 0
                                readonly property string architectureId: modelData
                                readonly property string architectureName: architectureInfoProvider.info.name || architectureId
                                readonly property var filteredEntries: filterEntries(searchTextField.text)
                                readonly property Instantiator singerInfoInstantiator: Instantiator {
                                    model: singerScrollView.mixedPage ? [] : architectureInfoProvider.singerIds
                                    onObjectAdded: architectureGroupBox.providerRevision++
                                    onObjectRemoved: architectureGroupBox.providerRevision++

                                    delegate: SingerInfoProvider {
                                        required property var modelData
                                        registry: CoreInterface.singerRegistry
                                        architectureId: architectureGroupBox.architectureId
                                        singerId: modelData
                                        onInfoChanged: architectureGroupBox.providerRevision++
                                        onExistsChanged: architectureGroupBox.providerRevision++
                                    }
                                }

                                Layout.fillWidth: true
                                leftPadding: 8
                                rightPadding: 8
                                title: architectureName
                                visible: filteredEntries.length > 0

                                function filterEntries(searchText) {
                                    providerRevision
                                    const query = searchText.trim().toLocaleLowerCase()
                                    const matches = text => String(text).toLocaleLowerCase().includes(query)
                                    const architectureMatches = matches(architectureId) || matches(architectureName)
                                    const result = []
                                    if (singerScrollView.mixedPage) {
                                        for (const entry of control.presetEntries) {
                                            const sources = entry.sourcesModel
                                            sources.revision
                                            if (sources.architectureId !== architectureId)
                                                continue
                                            if (control.mixGroup !== "" && sources.valid && sources.mixGroup !== control.mixGroup)
                                                continue
                                            if (architectureMatches || matches(entry.name))
                                                result.push(entry)
                                        }
                                    } else {
                                        for (let i = 0; i < singerInfoInstantiator.count; ++i) {
                                            const provider = singerInfoInstantiator.objectAt(i)
                                            if (!provider || !provider.exists)
                                                continue
                                            if (control.mixGroup !== "" && provider.info.mixGroup !== control.mixGroup)
                                                continue
                                            const name = provider.info.name || provider.singerId
                                            if (architectureMatches || matches(provider.singerId) || matches(name))
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
                                    width: architectureGroupBox.availableWidth
                                    spacing: 0

                                    Annotation {
                                        Layout.fillWidth: true
                                        Layout.bottomMargin: 8
                                        visible: !architectureInfoProvider.exists
                                        ThemedItem.controlType: SVS.CT_Warning
                                        label: qsTr('Architecture "%1" is unavailable.').arg(architectureGroupBox.architectureId)
                                    }

                                    Component {
                                        id: singleSingerDelegate

                                        T.Button {
                                            id: singerButton

                                            required property int index
                                            required property var modelData
                                            readonly property SingerInfoProvider singerInfoProvider: modelData
                                            property bool dragPayloadReady: false

                                            Layout.fillWidth: true
                                            height: 60
                                            Accessible.role: Accessible.ListItem
                                            Accessible.name: singerInfoProvider.info.name
                                                             || singerInfoProvider.singerId
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
                                            onClicked: {
                                                dragSourcesModel.fromDefaultSinger(singerInfoProvider.singerId)
                                                if (dragSourcesModel.valid)
                                                    control.singerSelected(dragSourcesModel)
                                            }

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
                                                                singerButton.singerInfoProvider.singerId)
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
                                                       || singerButton.singerInfoProvider.singerId
                                                subtitle: control.draggable && singerButton.hovered
                                                          ? qsTr("Drag to set singer")
                                                          : architectureGroupBox.architectureName
                                                image: SingerAvatar {
                                                    architectureId: architectureGroupBox.architectureId
                                                    singerTree: [singerButton.singerInfoProvider.singerId]
                                                }
                                            }
                                        }
                                    }

                                    Component {
                                        id: mixedSingerDelegate

                                        Item {
                                            id: singerCard

                                            required property int index
                                            required property var modelData
                                            readonly property SourcesPickerModel presetSources: modelData.sourcesModel
                                            readonly property bool singerValid: presetSources.valid
                                            readonly property string warningText: presetSources.validationIssues.map(issue => issue.message).join("\n")
                                            property bool dragPayloadReady: false

                                            Layout.fillWidth: true
                                            implicitHeight: Math.max(60, singerTitle.implicitHeight + subtitleRow.implicitHeight + 16)

                                            T.Button {
                                                id: singerButton
                                                anchors.fill: parent
                                                enabled: singerCard.singerValid
                                                text: singerCard.modelData.name
                                                Accessible.role: Accessible.ListItem

                                                Drag.keys: [control.sourcesPickerModelMimeType]
                                                Drag.mimeData: ({})
                                                Drag.supportedActions: Qt.CopyAction
                                                Drag.proposedAction: Qt.CopyAction
                                                Drag.dragType: Drag.Automatic
                                                Drag.active: enabled && control.draggable && singerDragHandler.active && singerCard.dragPayloadReady
                                                Drag.hotSpot.x: width / 2
                                                Drag.hotSpot.y: height / 2

                                                onClicked: {
                                                    if (singerCard.presetSources.valid)
                                                        control.singerSelected(singerCard.presetSources)
                                                }

                                                DragHandler {
                                                    id: singerDragHandler
                                                    enabled: singerButton.enabled && control.draggable
                                                    target: null
                                                    onActiveChanged: {
                                                        singerCard.dragPayloadReady = false
                                                        if (!active)
                                                            return
                                                        const sources = singerCard.presetSources
                                                        if (!sources.valid)
                                                            return
                                                        const data = sources.serialize()
                                                        if (data.length === 0)
                                                            return
                                                        const mimeData = {}
                                                        mimeData[control.sourcesPickerModelMimeType] = data
                                                        singerButton.Drag.mimeData = mimeData
                                                        singerCard.dragPayloadReady = true
                                                    }
                                                }

                                                Card {
                                                    id: singerCardView

                                                    property color _baseColor: Theme.backgroundColor(control.ThemedItem.backgroundLevel)
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

                                                    anchors.fill: parent
                                                    atTop: singerCard.index === 0
                                                    atBottom: singerCard.index === singerRepeater.count - 1
                                                    title: Label {
                                                        id: singerTitle
                                                        Layout.fillWidth: true
                                                        text: singerCard.modelData.name
                                                        elide: Text.ElideRight
                                                        font.weight: Font.DemiBold
                                                        color: singerCard.singerValid ? Theme.foregroundPrimaryColor
                                                                                     : Theme.foregroundDisabledColorChange.apply(Theme.foregroundPrimaryColor)
                                                    }
                                                    subtitle: RowLayout {
                                                        id: subtitleRow
                                                        Layout.fillWidth: true
                                                        spacing: 4

                                                        IconLabel {
                                                            visible: !singerCard.singerValid
                                                            icon.source: "image://fluent-system-icons/warning"
                                                            icon.color: Theme.warningColor
                                                        }

                                                        Label {
                                                            Layout.fillWidth: true
                                                            elide: Text.ElideRight
                                                            color: singerCard.singerValid ? Theme.foregroundSecondaryColor : Theme.warningColor
                                                            text: !singerCard.singerValid ? singerCard.warningText.replace(/\n/g, " ")
                                                                  : control.draggable && singerButton.hovered ? qsTr("Drag to set singer")
                                                                  : architectureGroupBox.architectureName
                                                        }
                                                    }
                                                    image: SingerAvatar {
                                                        architectureId: architectureGroupBox.architectureId
                                                        singerTree: {
                                                            singerCard.presetSources.revision
                                                            return singerCard.presetSources.data(singerCard.presetSources.modelIndex(0),
                                                                                                SourcesPickerModel.SingerTreeRole)
                                                        }
                                                    }
                                                    toolBar: Item {
                                                        implicitWidth: presetToolBar.implicitWidth
                                                        implicitHeight: presetToolBar.implicitHeight
                                                    }

                                                    onColorChanged: background.color = color
                                                }
                                            }

                                            // The toolbar remains enabled when the singer button is disabled.
                                            RowLayout {
                                                id: presetToolBar
                                                anchors.right: parent.right
                                                anchors.rightMargin: 6
                                                anchors.verticalCenter: parent.verticalCenter
                                                spacing: 4

                                                HoverHandler {
                                                    id: presetToolBarHover
                                                }

                                                ToolButton {
                                                    text: qsTr("Rename Preset")
                                                    display: AbstractButton.IconOnly
                                                    icon.source: "image://fluent-system-icons/edit"
                                                    onClicked: {
                                                        presetNameDialog.presetId = singerCard.modelData.id
                                                        presetNameDialog.initialName = singerCard.modelData.name
                                                        presetNameDialog.open()
                                                    }
                                                }

                                                ToolButton {
                                                    text: qsTr("Delete Preset")
                                                    display: AbstractButton.IconOnly
                                                    icon.source: "image://fluent-system-icons/delete"
                                                    onClicked: CoreInterface.mixedSingerPresets.removePreset(singerCard.modelData.id)
                                                }
                                            }

                                            HoverHandler {
                                                id: singerCardHover
                                                enabled: !singerCard.singerValid
                                            }

                                            DescriptiveText.activated: singerCardHover.hovered
                                                                       && !presetToolBarHover.hovered
                                                                       && !singerCard.singerValid
                                            DescriptiveText.toolTip: singerCard.warningText
                                            DescriptiveText.bindAccessibleDescription: true
                                        }
                                    }

                                    Repeater {
                                        id: singerRepeater
                                        model: architectureGroupBox.filteredEntries
                                        delegate: singerScrollView.mixedPage ? mixedSingerDelegate : singleSingerDelegate
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
