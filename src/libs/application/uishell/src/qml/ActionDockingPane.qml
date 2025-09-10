import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents

import QActionKit

DockingPane {
    title: ActionInstantiator.text
    description: ActionInstantiator.description
    icon.source: ActionInstantiator.icon.source
    icon.width: 16
    icon.height: 16
    icon.color: ActionInstantiator.icon.color.valid ? ActionInstantiator.icon.color : Theme.foregroundPrimaryColor
    Docking.onWindowChanged: () => {
        Docking.window.StatusTextContext.statusContext = Docking.window.transientParent?.StatusTextContext.statusContext ?? null
        Docking.window.StatusTextContext.contextHelpContext = Docking.window.transientParent?.StatusTextContext.contextHelpContext ?? null
    }
}