import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

Dialog {
    id: dialog

    property int trackCount
    property int insertionIndex
    property int insertionCount

    title: qsTr("Insert Track")

    onAboutToShow: insertionIndexSpinBox.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2
        Label {
            id: insertionIndexLabel
            text: qsTr("Insert position")
        }
        SpinBox {
            id: insertionIndexSpinBox
            Accessible.labelledBy: insertionIndexLabel
            Accessible.name: insertionIndexLabel.text
            Layout.fillWidth: true
            value: dialog.insertionIndex + 1
            from: 1
            to: dialog.trackCount + 1
            onValueModified: dialog.insertionIndex = value - 1
        }
        Label {
            id: insertionCountLabel
            text: qsTr("Number of tracks")
        }
        SpinBox {
            id: insertionCountSpinBox
            Accessible.labelledBy: insertionCountLabel
            Accessible.name: insertionCountLabel.text
            Layout.fillWidth: true
            value: dialog.insertionCount
            from: 1
            // to: 10
            onValueModified: dialog.insertionCount = value
        }
    }
    standardButtons: DialogButtonBox.Ok
}