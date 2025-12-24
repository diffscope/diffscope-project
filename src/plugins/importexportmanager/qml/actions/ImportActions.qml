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
        actionId: "org.diffscope.importexportmanager.file.import"
        Menu {
            id: menu
            Instantiator {
                model: DelegateModel {
                    model: d.addOn?.importConverters ?? []
                    delegate: Action {
                        required property QtObject modelData
                        text: modelData.name
                        DescriptiveAction.statusTip: modelData.description
                        onTriggered: {
                            d.addOn.execImport(modelData)
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
}