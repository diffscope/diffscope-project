import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: d

    required property RecentFileAddOn addOn

    ActionItem {
        actionId: "org.diffscope.core.file.openRecentFile"
        Menu {
            id: recentFilesMenu
            Instantiator {
                model: DelegateModel {
                    model: d.addOn.recentFilesModel
                    delegate: Action {
                        required property int index
                        required property var modelData
                        text: `${index < 9 ? "&" + Qt.locale().toString(index + 1) + ". " : ""}${modelData.name} [${modelData.path}]`
                        onTriggered: (o) => {
                            Qt.callLater(() => CoreInterface.openFile(modelData.path, o.Window.window))
                        }
                    }
                }
                onObjectAdded: (index, object) => {
                    recentFilesMenu.insertAction(index, object)
                }
                onObjectRemoved: (index, object) => {
                    recentFilesMenu.removeAction(object)
                }
            }
            MenuSeparator {
            }
            Action {
                text: "Clear Recent Files"
                onTriggered: CoreInterface.recentFileCollection.clearRecentFile()
            }
        }
    }

}