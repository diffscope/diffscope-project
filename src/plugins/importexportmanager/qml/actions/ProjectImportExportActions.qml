import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents

import QActionKit

import DiffScope.UIShell

ActionCollection {
    id: d

    required property QtObject addOn

    ActionItem {
        actionId: "org.diffscope.importexportmanager.file.export"
        Menu {
            id: menu
            Instantiator {
                model: DelegateModel {
                    model: d.addOn?.exportConverters ?? []
                    delegate: Action {
                        required property QtObject modelData
                        text: modelData.name
                        DescriptiveAction.statusTip: modelData.description
                        onTriggered: Qt.callLater(() => d.addOn.execExport(modelData))
                    }
                }
                onObjectAdded: (index, object) => {
                    menu.insertAction(index, object)
                }
                onObjectRemoved: (index, object) => {
                    menu.removeAction(object)
                }
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.importexportmanager.project.importAsTracks"
        Menu {
            id: importTracksMenu
            Instantiator {
                model: DelegateModel {
                    model: d.addOn?.importConverters ?? []
                    delegate: Action {
                        required property QtObject modelData
                        text: modelData?.name ?? ""
                        DescriptiveAction.statusTip: modelData?.description ?? ""
                        onTriggered: Qt.callLater(() => d.addOn.execImportTracks(modelData))
                    }
                }
                onObjectAdded: (index, object) => {
                    importTracksMenu.insertAction(index, object)
                }
                onObjectRemoved: (index, object) => {
                    importTracksMenu.removeAction(object)
                }
            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.importexportmanager.edit.copySpecial"
        Action {
            onTriggered: () => {

            }
        }
    }
    ActionItem {
        actionId: "org.diffscope.importexportmanager.edit.pasteSpecial"
        Action {
            onTriggered: () => {

            }
        }
    }
}
