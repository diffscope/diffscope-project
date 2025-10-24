import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: d

    required property ProjectWindowInterface windowHandle
    property Window window: windowHandle?.window ?? null

    ActionItem {
        actionId: "core.file.save"
        Action {
            enabled: d.windowHandle.projectDocumentContext.fileLocker
            onTriggered: Qt.callLater(() => d.windowHandle.save())
        }
    }

    ActionItem {
        actionId: "core.file.saveAs"
        Action {
            enabled: d.windowHandle.projectDocumentContext.fileLocker
            onTriggered: Qt.callLater(() => d.windowHandle.saveAs())
        }
    }

    ActionItem {
        actionId: "core.file.saveCopy"
        Action {
            onTriggered: Qt.callLater(() => d.windowHandle.saveCopy())
        }
    }

    ActionItem {
        actionId: "core.statusText"
        Label {
            text: d.window.StatusTextContext.statusContext.text
            TapHandler {
                enabled: Boolean(d.window.StatusTextContext.statusContext.contextObject)
                onSingleTapped: () => {
                    d.window.StatusTextContext.statusContext.contextObject._diffscope_statusTipTriggered()
                }
            }
            HoverHandler {
                id: hoverHandler
                enabled: Boolean(d.window.StatusTextContext.statusContext.contextObject)
                cursorShape: Qt.PointingHandCursor
            }
            DescriptiveText.activated: hoverHandler.hovered
            DescriptiveText.toolTip: qsTr("Click to show details")
        }
    }

}