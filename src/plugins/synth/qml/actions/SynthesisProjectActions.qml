pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

ActionCollection {
    id: root

    required property QtObject addOn

    ActionItem {
        actionId: "org.diffscope.synth.status"

        Label {
            leftPadding: 6
            rightPadding: 6
            text: qsTr("Synthesis: %1 running, %2 queued").arg(root.addOn.synthesisContext.synthesizingPieceCount).arg(root.addOn.synthesisContext.queuedPieceCount)
            Accessible.name: text
            ThemedItem.foregroundLevel: root.addOn.synthesisContext.synthesizingPieceCount > 0 ? SVS.FL_Primary : SVS.FL_Secondary
        }
    }
}
