import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn

    readonly property Component arrangementPanelComponent: ActionDockingPane {
        data: [d.addOn?.arrangementPanelInterface.arrangementView ?? null]
    }

}