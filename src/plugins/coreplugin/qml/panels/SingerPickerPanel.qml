import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

ActionDockingPane {
    id: pane

    signal singerSelected(string architectureId, string singerId)

    SingerPicker {
        anchors.fill: parent
        draggable: true
        onSingerSelected: (architectureId, singerId) => {
            pane.singerSelected(architectureId, singerId)
        }
    }
}
