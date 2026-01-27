import QtQml
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
        ColumnLayout {
            anchors.fill: parent
            spacing: 12
            Label {
                ThemedItem.foregroundLevel: SVS.FL_Secondary
                Layout.fillWidth: true
                Layout.topMargin: 12
                horizontalAlignment: Qt.AlignHCenter
                text: qsTr("Select an item to edit properties")
                wrapMode: Text.Wrap
                visible: tabBarStackLayout.currentIndex === 0
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Layout.topMargin: 12
                visible: tabBarStackLayout.currentIndex !== 0
                StackLayout {
                    id: tabBarStackLayout
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
                    Item {
                        id: noneTabBar
                    }
                    TabBar {
                        id: anchorNodeTabBar
                        ThemedItem.flat: true
                        onCurrentIndexChanged: propertyEditorStackLayout.currentIndex = [1, 2, 6][currentIndex]
                        TabButton {
                            text: qsTr("Anchor Node")
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                            icon.source: "image://fluent-system-icons/data_line"
                        }
                        TabButton {
                            text: qsTr("Clip")
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                            icon.source: "image://fluent-system-icons/gantt_chart"
                        }
                        TabButton {
                            text: qsTr("Track")
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                            icon.source: "image://fluent-system-icons/list_bar"
                        }
                    }
                    TabBar {
                        id: clipTabBar
                        ThemedItem.flat: true
                        onCurrentIndexChanged: propertyEditorStackLayout.currentIndex = [2, 6][currentIndex]
                        TabButton {
                            text: qsTr("Clip")
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                            icon.source: "image://fluent-system-icons/gantt_chart"
                        }
                        TabButton {
                            text: qsTr("Track")
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                            icon.source: "image://fluent-system-icons/list_bar"
                        }
                    }
                    TabBar {
                        id: labelTabBar
                        ThemedItem.flat: true
                        onCurrentIndexChanged: propertyEditorStackLayout.currentIndex = [3][currentIndex]
                        TabButton {
                            text: qsTr("Label")
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                            icon.source: "image://fluent-system-icons/tag"
                        }
                    }
                    TabBar {
                        id: noteTabBar
                        ThemedItem.flat: true
                        onCurrentIndexChanged: propertyEditorStackLayout.currentIndex = [4, 2, 6][currentIndex]
                        TabButton {
                            text: qsTr("Note")
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                            icon.source: "image://fluent-system-icons/music_note_1"
                        }
                        TabButton {
                            text: qsTr("Clip")
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                            icon.source: "image://fluent-system-icons/gantt_chart"
                        }
                        TabButton {
                            text: qsTr("Track")
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                            icon.source: "image://fluent-system-icons/list_bar"
                        }
                    }
                    TabBar {
                        id: tempoTabBar
                        ThemedItem.flat: true
                        onCurrentIndexChanged: propertyEditorStackLayout.currentIndex = [5][currentIndex]
                        TabButton {
                            text: qsTr("Tempo")
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                        }
                    }
                    TabBar {
                        id: trackTabBar
                        ThemedItem.flat: true
                        onCurrentIndexChanged: propertyEditorStackLayout.currentIndex = [6][currentIndex]
                        TabButton {
                            text: qsTr("Track")
                            ThemedItem.tabIndicator: SVS.TI_Bottom
                            width: implicitWidth
                            icon.source: "image://fluent-system-icons/list_bar"
                        }
                    }
                }
            }
            ScrollView {
                id: scrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                ColumnLayout {
                    width: scrollView.width - 24
                    x: 12
                    StackLayout {
                        id: propertyEditorStackLayout
                        Layout.fillWidth: true
                        Layout.bottomMargin: 12
                        Repeater {
                            model: [null, "anchorNodeComponents", "clipComponents", "labelComponents", "noteComponents", "tempoComponents", "trackComponents"]
                            delegate: ColumnLayout {
                                id: propertyEditor
                                required property var modelData
                                spacing: 12
                                Repeater {
                                    model: propertyEditor.modelData ? CoreInterface.propertyEditorManager[propertyEditor.modelData] : null
                                    delegate: StackLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: item?.implicitHeight ?? 0
                                        property Item item: null
                                        data: [item]
                                        Component.onCompleted: () => {
                                            item = modelData.createObject(this, {
                                                windowHandle: d.addOn?.windowHandle ?? null,
                                                propertyMapper: null
                                            })
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