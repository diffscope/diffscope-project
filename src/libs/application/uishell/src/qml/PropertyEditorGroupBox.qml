import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

GroupBox {
    id: groupBox

    property bool contentVisible: contentOpacity !== 0
    property double contentOpacity: groupBox.ThemedItem.folded ? 0 : 1
    property double contentAnimatedHeight: contentHeight
    property bool visualVisible: true
    ThemedItem.foldable: true
    clip: true
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentAnimatedHeight + topPadding + bottomPadding)

    Behavior on contentOpacity {
        NumberAnimation {
            duration: Theme.visualEffectAnimationDuration
            easing.type: Easing.OutCubic
        }
    }

    NumberAnimation on contentAnimatedHeight {
        id: collapseAnimation
        to: 0
        duration: Theme.visualEffectAnimationDuration
        easing.type: Easing.OutCubic
    }

    NumberAnimation on contentAnimatedHeight {
        id: expandAnimation
        to: groupBox.contentHeight
        duration: Theme.visualEffectAnimationDuration
        easing.type: Easing.OutCubic
    }

    ThemedItem.onFoldedChanged: {
        if (groupBox.ThemedItem.folded) {
            collapseAnimation.start()
        } else {
            expandAnimation.start()
        }
    }

    onContentHeightChanged: {
        collapseAnimation.stop()
        expandAnimation.stop()
        contentAnimatedHeight = contentHeight
    }

    contentItem.visible: contentVisible
    contentItem.opacity: contentOpacity
}