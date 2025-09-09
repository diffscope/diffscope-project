import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import QActionKit

import DiffScope.UIShell
import DiffScope.CorePlugin

ActionCollection {
    id: d

    required property IProjectWindow windowHandle
    property Window window: windowHandle?.window ?? null

    ActionItem {
        actionId: "core.statusText"
        Label {
            text: d.window.StatusTextContext.statusContext.text
        }
    }

}