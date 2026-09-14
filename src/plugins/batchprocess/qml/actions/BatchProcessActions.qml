// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.BatchProcess
import DiffScope.Core

ActionCollection {
    id: d

    required property BatchProcessAddOn addOn

    ActionItem {
        actionId: "org.diffscope.batchprocess.batchProcess"
        Menu {
            id: batchProcessMenu

            Instantiator {
                model: DelegateModel {
                    model: d.addOn.actionModel
                    delegate: DelegateChooser {
                        role: "separator"
                        DelegateChoice {
                            roleValue: true
                            MenuSeparator {
                            }
                        }
                        DelegateChoice {
                            roleValue: false
                            Action {
                                required property string name
                                required property string description
                                required property var scriptAction

                                text: name
                                onTriggered: Qt.callLater(() => d.addOn.executeAction(scriptAction))
                            }
                        }
                    }
                }
                onObjectAdded: (index, object) => {
                    if (object instanceof Action) {
                        batchProcessMenu.insertAction(index, object)
                    } else {
                        batchProcessMenu.insertItem(index, object)
                    }
                }
                onObjectRemoved: (index, object) => {
                    if (object instanceof Action) {
                        batchProcessMenu.removeAction(object)
                    } else {
                        batchProcessMenu.removeItem(object)
                    }
                }
            }

            Action {
                text: qsTr("Reload Scripts")
                onTriggered: Qt.callLater(() => d.addOn.reloadScripts())
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.batchprocess.reloadScripts"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.reloadScripts())
        }
    }
}
