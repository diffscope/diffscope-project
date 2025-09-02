import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.UIShell
import DiffScope.CorePlugin

ActionCollection {
    id: d

    required property TimelineAddOn addOn
    readonly property IProjectWindow windowHandle: addOn?.windowHandle ?? null
    readonly property Window window: addOn?.windowHandle.window ?? null

    ActionItem {
        actionId: "core.digitalClock"
        DigitalClock {
            id: digitalClock
            backgroundVisible: BehaviorPreference.timeIndicatorBackgroundVisible
            text: d.addOn.showMusicTime ? d.addOn.musicTimeText : d.addOn.longTimeText
            function doInteraction(flag) {
                if (flag === BehaviorPreference.TIIB_ToggleFormat) {
                    d.addOn.showMusicTime = !d.addOn.showMusicTime
                } else if (flag === BehaviorPreference.TIIB_ShowGoTo) {
                    d.windowHandle.triggerAction("core.timelineGoTo", this)
                } else if (flag === BehaviorPreference.TIIB_ShowQuickJump) {
                    d.addOn.execQuickJump(text)
                }
            }
            Menu {
                id: digitalClockContextMenu
                MenuActionInstantiator {
                    actionId: "core.timeline"
                    context: d.windowHandle?.actionContext ?? null
                    Component.onCompleted: forceUpdateLayouts()
                }
            }
            TapHandler {
                acceptedButtons: Qt.RightButton
                onSingleTapped: () => {
                    digitalClockContextMenu.popup(digitalClock)
                }
            }
            Timer {
                id: clickTimer
                onTriggered: digitalClock.doInteraction(BehaviorPreference.timeIndicatorClickAction)
            }
            Popup {
                id: sliderPopup
                width: 240
                x: (parent.width - width) / 2
                y: parent.height
                padding: 0
                Frame {
                    id: sliderPopupFrame
                    background: null
                    padding: 4
                    hoverEnabled: true
                    width: 240
                    Slider {
                        id: slider
                        anchors.fill: parent
                        from: 0
                        to: d.windowHandle.projectTimeline.rangeHint - 1
                        stepSize: 1
                        snapMode: Slider.SnapAlways
                        value: d.windowHandle.projectTimeline.position
                        onMoved: () => {
                            d.windowHandle.projectTimeline.position = d.windowHandle.projectTimeline.lastPosition = value
                        }
                    }
                    onHoveredChanged: () => {
                        if (!hovered && !digitalClock.hovered) {
                            sliderPopup.visible = false
                        }
                    }
                }
            }
            onHoveredChanged: () => {
                if (!BehaviorPreference.timeIndicatorShowSliderOnHover) {
                    return
                }
                if (hovered) {
                    sliderPopup.visible = true
                } else {
                    if (!sliderPopupFrame.hovered) {
                        sliderPopup.visible = false
                    }
                }
            }
            onClicked: () => {
                clickTimer.interval = d.addOn.doubleClickInterval
                clickTimer.start()
            }
            onDoubleClicked: () => {
                clickTimer.stop()
                doInteraction(BehaviorPreference.timeIndicatorDoubleClickAction)
            }
            onPressAndHold: doInteraction(BehaviorPreference.timeIndicatorPressAndHoldAction)
        }
    }

    ActionItem {
        actionId: "core.timeIndicatorShowMusicTime"
        Action {
            checkable: true
            checked: d.addOn.showMusicTime
            onTriggered: d.addOn.showMusicTime = checked
        }
    }

    ActionItem {
        actionId: "core.timeIndicatorShowAbsoluteTime"
        Action {
            checkable: true
            checked: d.addOn.showAbsoluteTime
            onTriggered: d.addOn.showAbsoluteTime = checked
        }
    }

    ActionItem {
        actionId: "core.timelineGoTo"
        Action {

        }
    }

    ActionItem {
        actionId: "core.timelineQuickJump"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.execQuickJump())
        }
    }

    ActionItem {
        actionId: "core.timelineGoToStart"
        Action {

        }
    }

    ActionItem {
        actionId: "core.timelineGoToPreviousMeasure"
        Action {

        }
    }

    ActionItem {
        actionId: "core.timelineGoToPreviousBeat"
        Action {

        }
    }

    ActionItem {
        actionId: "core.timelineGoToPreviousTick"
        Action {

        }
    }

    ActionItem {
        actionId: "core.timelineGoToEnd"
        Action {

        }
    }

    ActionItem {
        actionId: "core.timelineGoToNextMeasure"
        Action {

        }
    }

    ActionItem {
        actionId: "core.timelineGoToNextBeat"
        Action {

        }
    }

    ActionItem {
        actionId: "core.timelineGoToNextTick"
        Action {

        }
    }

    ActionItem {
        actionId: "core.resetProjectTimeRange"
        Action {

        }
    }


}