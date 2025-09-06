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
}