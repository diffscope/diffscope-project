import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

ColumnLayout {
    id: d

    required property QtObject additionalTrackLoader

    readonly property Repeater additionalTrackRepeater: additionalTrackRepeater

    spacing: 0

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: Theme.paneSeparatorColor
    }
    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: false
        spacing: 0
        Repeater {
            id: additionalTrackRepeater
            model: d.additionalTrackLoader?.loadedComponents ?? []
            ColumnLayout {
                required property string modelData
                required property int index
                Layout.fillWidth: true
                spacing: 0
                readonly property Item separator: Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: Theme.paneSeparatorColor
                }
                readonly property Item item: d.additionalTrackLoader?.loadedItems[index] ?? null
                data: [item, separator]
            }
        }
    }
}