import QtQml
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
                        onTriggered: {
                            d.addOn.execExport(modelData)
                        }
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
        Action {
            onTriggered: () => {

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