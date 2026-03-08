import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn
    property bool isRecovery: false
    readonly property QtObject windowHandle: addOn?.windowHandle ?? null

    readonly property Component recentFilesPanelComponent: ActionDockingPane {
        id: pane

        property bool recentFilesIsListView: false

        ThemedItem.backgroundLevel: d.addOn?.isHomeWindow ? SVS.BL_Quaternary : SVS.BL_Primary
        badgeType: SVS.CT_Warning
        badgeNumber: d.isRecovery ? (d.addOn?.recoveryFilesModel.rowCount() ?? 0) : 0

        function loadState(state) {
            recentFilesIsListView = state.recentFilesIsListView
        }

        function saveState() {
            return {
                recentFilesIsListView: pane.recentFilesIsListView
            }
        }

        header: RowLayout {
            anchors.fill: parent
            spacing: 8
            TextField {
                id: searchTextField
                placeholderText: qsTr("Search")
                Accessible.name: qsTr("Search")
                Layout.fillWidth: true
                ThemedItem.icon.source: "image://fluent-system-icons/search"
                implicitHeight: 24
            }
            RowLayout {
                visible: !d.isRecovery
                ToolButton {
                    icon.source: "image://fluent-system-icons/grid"
                    checkable: true
                    autoExclusive: true
                    checked: !pane.recentFilesIsListView
                    onClicked: GlobalHelper.setProperty(pane, "recentFilesIsListView", !checked)
                    text: qsTr("Grid view")
                    display: AbstractButton.IconOnly
                }
                ToolButton {
                    icon.source: "image://fluent-system-icons/list"
                    checkable: true
                    autoExclusive: true
                    checked: pane.recentFilesIsListView
                    onClicked: GlobalHelper.setProperty(pane, "recentFilesIsListView", checked)
                    text: qsTr("List view")
                    display: AbstractButton.IconOnly
                }
            }
        }

        menu: Menu {
            Action {
                text: d.isRecovery ? qsTr("Clear Recovery Files") : qsTr("Clear Recent Files")
                onTriggered: CoreInterface.recentFileCollection.clearRecentFile()
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            anchors.topMargin: d.addOn?.isHomeWindow ? 0 : 16
            spacing: 16
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                visible: Boolean(d.addOn?.isHomeWindow)
                TextField {
                    id: searchTextField2
                    placeholderText: qsTr("Search")
                    Accessible.name: qsTr("Search")
                    Layout.fillWidth: true
                    ThemedItem.icon.source: "image://fluent-system-icons/search"
                }
                RowLayout {
                    visible: !d.isRecovery
                    ToolButton {
                        icon.source: "image://fluent-system-icons/grid"
                        checkable: true
                        autoExclusive: true
                        checked: !pane.recentFilesIsListView
                        onClicked: GlobalHelper.setProperty(pane, "recentFilesIsListView", !checked)
                        text: qsTr("Grid view")
                        display: AbstractButton.IconOnly
                    }
                    ToolButton {
                        icon.source: "image://fluent-system-icons/list"
                        checkable: true
                        autoExclusive: true
                        checked: pane.recentFilesIsListView
                        onClicked: GlobalHelper.setProperty(pane, "recentFilesIsListView", checked)
                        text: qsTr("List view")
                        display: AbstractButton.IconOnly
                    }
                }
            }
            FileView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                searchText: d.addOn?.isHomeWindow ? searchTextField2.text : searchTextField.text
                filesModel: d.isRecovery ? (d.addOn?.recoveryFilesModel ?? null) : (d.addOn?.recentFilesModel ?? null)
                displayMode: pane.recentFilesIsListView || d.isRecovery ? FileView.List : FileView.Grid
                newFileActionEnabled: Boolean(d.addOn?.isHomeWindow && !d.isRecovery)
                emptyTip: d.isRecovery ? qsTr("No recovery file\nIf %1 exits abnormally, automatic recovery files will be displayed here.").arg(Application.displayName) : qsTr("No recent files")

                onNewFileRequested: d.windowHandle.triggerAction("org.diffscope.core.file.new", this)
                onOpenFileRequested: (index) => {
                    if (d.isRecovery) {
                        // TODO
                        console.log("TODO: open recovery file ", index)
                    } else {
                        d.addOn.openRecentFile(index)
                    }
                }
                onContextMenuRequested: (index) => {
                    fileMenu.index = index
                    fileMenu.path = filesModel.data(filesModel.index(index, 0), USDef.RF_PathRole)
                    fileMenu.popup()
                }

                Menu {
                    id: fileMenu
                    property int index
                    property string path
                    Action {
                        text: qsTr("Open")
                        icon.source: "image://fluent-system-icons/folder_open"
                        onTriggered: () => {
                            if (d.isRecovery) {
                                // TODO
                                console.log("TODO: open recovery file ", fileMenu.index)
                            } else {
                                d.addOn.openRecentFile(fileMenu.index)
                            }
                        }
                    }
                    Action {
                        text: qsTr("Reveal in %1").arg(DesktopServices.fileManagerName)
                        icon.source: "image://fluent-system-icons/open_folder"
                        enabled: fileMenu.path.length !== 0
                        onTriggered: () => {
                            DesktopServices.reveal(fileMenu.path)
                        }
                    }
                    Action {
                        text: d.isRecovery ? qsTr('Remove from "Recovery Files"') : qsTr('Remove from "Recent Files"')
                        icon.source: "image://fluent-system-icons/document_dismiss"
                        onTriggered: () => {
                            if (d.isRecovery) {
                                // TODO
                                console.log("TODO: remove recovery file ", fileMenu.index)
                            } else {
                                d.addOn.removeRecentFile(fileMenu.index)
                            }
                        }
                    }
                }
            }
        }


    }

}