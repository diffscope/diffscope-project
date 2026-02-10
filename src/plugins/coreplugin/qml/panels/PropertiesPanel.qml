import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn

    readonly property Component propertiesPanelComponent: ActionDockingPane {
        ListModel {
            id: selectionTypeModel
            ListElement {
                text: qsTr("Document")
                iconSource: "image://fluent-system-icons/document"
                componentsKey: "noneComponents"
                propertyMapperKey: "nonePropertyMapper"
            }
            ListElement {
                text: qsTr("Anchor Node")
                iconSource: "image://fluent-system-icons/automation_3p"
                componentsKey: "anchorNodeComponents"
                propertyMapperKey: "anchorNodePropertyMapper"
            }
            ListElement {
                text: qsTr("Clip")
                iconSource: "image://fluent-system-icons/gantt_chart"
                componentsKey: "clipComponents"
                propertyMapperKey: "clipPropertyMapper"
            }
            ListElement {
                text: qsTr("Label")
                iconSource: "image://fluent-system-icons/tag"
                componentsKey: "labelComponents"
                propertyMapperKey: "labelPropertyMapper"
            }
            ListElement {
                text: qsTr("Note")
                iconSource: "image://fluent-system-icons/music_note_1"
                componentsKey: "noteComponents"
                propertyMapperKey: "labelPropertyMapper"
            }
            ListElement {
                text: qsTr("Tempo")
                iconSource: "image://fluent-system-icons/metronome"
                componentsKey: "tempoComponents"
                propertyMapperKey: "tempoPropertyMapper"
            }
            ListElement {
                text: qsTr("Track")
                iconSource: "image://fluent-system-icons/list_bar"
                componentsKey: "trackComponents"
                propertyMapperKey: "trackPropertyMapper"
            }
        }
        QtObject {
            id: propertyMappers
            readonly property QtObject nonePropertyMapper: null
            readonly property QtObject anchorNodePropertyMapper: null
            readonly property QtObject clipPropertyMapper: ClipPropertyMapper {
                selectionModel: d.addOn?.windowHandle.projectDocumentContext.document.selectionModel ?? null
                readonly property bool inactive: ![1, 2, 4].includes(selectionModel?.selectionType) || !selectionModel?.selectedCount
            }
            readonly property QtObject labelPropertyMapper: LabelPropertyMapper {
                selectionModel: d.addOn?.windowHandle.projectDocumentContext.document.selectionModel ?? null
                readonly property bool inactive: selectionModel?.selectionType !== 3 || !selectionModel?.selectedCount
            }
            readonly property QtObject notePropertyMapper: null
            readonly property QtObject tempoPropertyMapper: TempoPropertyMapper {
                selectionModel: d.addOn?.windowHandle.projectDocumentContext.document.selectionModel ?? null
                readonly property bool inactive: selectionModel?.selectionType !== 5 || !selectionModel?.selectedCount
            }
            readonly property QtObject trackPropertyMapper: TrackPropertyMapper {
                selectionModel: d.addOn?.windowHandle.projectDocumentContext.document.selectionModel ?? null
                readonly property bool inactive: ![1, 2, 4, 6].includes(selectionModel?.selectionType) || !selectionModel?.selectedCount
            }
        }
        StackLayout {
            id: tabBarStackLayout
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 12
            currentIndex: {
                let selectionModel = d.addOn?.windowHandle.projectDocumentContext.document.selectionModel
                if (!selectionModel)
                    return 0
                let t = selectionModel.selectionType
                if (selectionModel.selectedCount !== 0) {
                    return t
                } else {
                    return [0, 2, 0, 0, 2, 0, 0][t]
                }
            }
            onCurrentIndexChanged: () => {
                let tabBar = itemAt(currentIndex)
                if (!(tabBar instanceof TabBar)) {
                    propertyEditorStackLayout.currentIndex = 0
                } else {
                    tabBar.currentIndex = 0
                    tabBar.currentIndexChanged()
                }
            }
            Component.onCompleted: propertyEditorStackLayout.currentIndex = 0
            Repeater {
                model: [
                    [0], [1, 2, 6, 0], [2, 6, 0], [3, 0], [4, 2, 6, 0], [5, 0], [6, 0]
                ]
                delegate: TabBar {
                    id: tabBar
                    required property var modelData
                    ThemedItem.flat: true
                    onCurrentIndexChanged: propertyEditorStackLayout.currentIndex = modelData[currentIndex]
                    Layout.preferredHeight: 30
                    Repeater {
                        model: tabBar.modelData
                        delegate: TabButton {
                            required property int modelData
                            text: selectionTypeModel.get(modelData).text
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                            icon.source: selectionTypeModel.get(modelData).iconSource
                        }
                    }
                }
            }
        }
        StackLayout {
            id: propertyEditorStackLayout
            anchors.top: tabBarStackLayout.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.topMargin: 12
            Repeater {
                model: selectionTypeModel
                delegate: ScrollView {
                    id: propertyEditorScrollView
                    required property var modelData
                    Layout.fillWidth: true
                    ColumnLayout {
                        width: propertyEditorScrollView.width - 24
                        x: 12
                        Repeater {
                            model: CoreInterface.propertyEditorManager[propertyEditorScrollView.modelData.componentsKey]
                            delegate: Item {
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.bottomMargin: 12
                                implicitHeight: item?.height ?? 0
                                property Item item: null
                                data: [item]
                                visible: item?.visualVisible ?? true
                                Component.onCompleted: () => {
                                    item = modelData.createObject(this, {
                                        windowHandle: d.addOn?.windowHandle ?? null,
                                        propertyMapper: propertyMappers[propertyEditorScrollView.modelData.propertyMapperKey]
                                    })
                                    item.width = Qt.binding(() => width)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}