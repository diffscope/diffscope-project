// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.UIShell
import DiffScope.DspxModel.SelectionModel as DspxSelectionModel

Item {
    id: root

    required property ProjectWindowInterface windowHandle

    function popupDocumentContextMenu(selectionType, sceneContextMenu) {
        let menu = null
        if (sceneContextMenu) {
            if (selectionType === DspxSelectionModel.SelectionModel.ST_Clip)
                menu = clipSceneContextMenu
            else if (selectionType === DspxSelectionModel.SelectionModel.ST_KeySignature)
                menu = keySignatureSceneContextMenu
            else if (selectionType === DspxSelectionModel.SelectionModel.ST_Label)
                menu = labelSceneContextMenu
            else if (selectionType === DspxSelectionModel.SelectionModel.ST_Tempo)
                menu = tempoSceneContextMenu
            else if (selectionType === DspxSelectionModel.SelectionModel.ST_Track)
                menu = trackSceneContextMenu
            else if (selectionType === DspxSelectionModel.SelectionModel.ST_Note)
                menu = noteSceneContextMenu
        } else {
            if (selectionType === DspxSelectionModel.SelectionModel.ST_Clip)
                menu = clipItemContextMenu
            else if (selectionType === DspxSelectionModel.SelectionModel.ST_Label)
                menu = labelItemContextMenu
            else if (selectionType === DspxSelectionModel.SelectionModel.ST_Tempo)
                menu = tempoItemContextMenu
            else if (selectionType === DspxSelectionModel.SelectionModel.ST_Track)
                menu = trackItemContextMenu
            else if (selectionType === DspxSelectionModel.SelectionModel.ST_Note)
                menu = noteItemContextMenu
        }
        // TODO: Support all selection types
        menu?.popup()
    }

    Menu {
        id: clipItemContextMenu
        popupType: Popup.Window

        MenuActionInstantiator {
            actionId: "org.diffscope.core.clipItemContextMenu"
            context: root.windowHandle?.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: labelItemContextMenu
        popupType: Popup.Window

        MenuActionInstantiator {
            actionId: "org.diffscope.core.labelItemContextMenu"
            context: root.windowHandle?.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: tempoItemContextMenu
        popupType: Popup.Window

        MenuActionInstantiator {
            actionId: "org.diffscope.core.tempoItemContextMenu"
            context: root.windowHandle?.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: trackItemContextMenu
        popupType: Popup.Window

        MenuActionInstantiator {
            actionId: "org.diffscope.core.trackItemContextMenu"
            context: root.windowHandle?.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: noteItemContextMenu
        popupType: Popup.Window

        MenuActionInstantiator {
            actionId: "org.diffscope.core.noteItemContextMenu"
            context: root.windowHandle?.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: clipSceneContextMenu
        popupType: Popup.Window

        MenuActionInstantiator {
            actionId: "org.diffscope.core.clipSceneContextMenu"
            context: root.windowHandle?.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: keySignatureSceneContextMenu
        popupType: Popup.Window

        MenuActionInstantiator {
            actionId: "org.diffscope.core.keySignatureSceneContextMenu"
            context: root.windowHandle?.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: labelSceneContextMenu
        popupType: Popup.Window

        MenuActionInstantiator {
            actionId: "org.diffscope.core.labelSceneContextMenu"
            context: root.windowHandle?.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: tempoSceneContextMenu
        popupType: Popup.Window

        MenuActionInstantiator {
            actionId: "org.diffscope.core.tempoSceneContextMenu"
            context: root.windowHandle?.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: trackSceneContextMenu
        popupType: Popup.Window

        MenuActionInstantiator {
            actionId: "org.diffscope.core.trackSceneContextMenu"
            context: root.windowHandle?.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }

    Menu {
        id: noteSceneContextMenu
        popupType: Popup.Window

        MenuActionInstantiator {
            actionId: "org.diffscope.core.noteSceneContextMenu"
            context: root.windowHandle?.actionContext ?? null
            Component.onCompleted: forceUpdateLayouts()
        }
    }
}
