import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

GroupBox {
    id: groupBox

    property bool contentVisible: contentOpacity !== 0
    property double contentOpacity: groupBox.ThemedItem.folded ? 0 : 1
    property double contentAnimatedHeight: groupBox.ThemedItem.folded ? 0 : contentHeight
    ThemedItem.foldable: true
    clip: true
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentAnimatedHeight + topPadding + bottomPadding)

    Behavior on contentOpacity {
        NumberAnimation {
            duration: Theme.visualEffectAnimationDuration
            easing.type: Easing.OutCubic
        }
    }
    Behavior on contentAnimatedHeight {
        NumberAnimation {
            duration: Theme.visualEffectAnimationDuration
            easing.type: Easing.OutCubic
        }
    }

    contentItem.visible: contentVisible
    contentItem.opacity: contentOpacity
}