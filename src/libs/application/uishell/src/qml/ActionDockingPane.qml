import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents

import QActionKit

DockingPane {
    title: ActionInstantiator.text
    description: ActionInstantiator.description
    icon.source: ActionInstantiator.iconSource
    icon.width: 16
    icon.height: 16
    icon.color: Theme.foregroundPrimaryColor
}