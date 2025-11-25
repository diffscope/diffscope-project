import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Layouts

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn

    readonly property Component tempoTrackComponent: Item {
        required property QtObject contextObject
        Layout.fillWidth: true
        implicitHeight: 16
        // TODO
    }

}