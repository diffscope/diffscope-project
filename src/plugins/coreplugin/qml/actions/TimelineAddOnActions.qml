import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: d

    required property TimelineAddOn addOn
    readonly property IProjectWindow windowHandle: addOn?.windowHandle ?? null
    readonly property Window window: addOn?.windowHandle.window ?? null

    ActionItem {
        actionId: "core.widget.digitalClock"
        DigitalClock {
            id: digitalClock
            backgroundVisible: BehaviorPreference.timeIndicatorBackgroundVisible
            text: d.addOn.showMusicTime ? d.addOn.musicTimeText : d.addOn.longTimeText
            DescriptiveText.activated: hovered
            DescriptiveText.statusTip: qsTr("Current project time")
            DescriptiveText.bindAccessibleDescription: true
            function doInteraction(flag) {
                if (flag === BehaviorPreference.TIIB_ToggleFormat) {
                    d.addOn.showMusicTime = !d.addOn.showMusicTime
                } else if (flag === BehaviorPreference.TIIB_ShowGoTo) {
                    d.windowHandle.triggerAction("core.timeline.goTo", this)
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
                    DescriptiveText.activated: hovered
                    DescriptiveText.statusTip: qsTr("Slide to adjust current project time")
                    DescriptiveText.bindAccessibleDescription: true
                    Slider {
                        id: slider
                        anchors.fill: parent
                        from: 0
                        to: d.windowHandle.projectTimeline.rangeHint - 1
                        stepSize: 1
                        snapMode: Slider.SnapAlways
                        value: d.windowHandle.projectTimeline.position
                        onMoved: () => {
                            d.windowHandle.projectTimeline.goTo(value)
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
        actionId: "core.timeIndicator.showMusicTime"
        Action {
            checkable: true
            checked: d.addOn.showMusicTime
            onTriggered: d.addOn.showMusicTime = checked
        }
    }

    ActionItem {
        actionId: "core.timeIndicator.showAbsoluteTime"
        Action {
            checkable: true
            checked: d.addOn.showAbsoluteTime
            onTriggered: d.addOn.showAbsoluteTime = checked
        }
    }

    ActionItem {
        actionId: "core.timeline.goTo"
        Action {

        }
    }

    ActionItem {
        actionId: "core.timeline.quickJump"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.execQuickJump())
        }
    }

    ActionItem {
        actionId: "core.timeline.goToStart"
        Action {
            onTriggered: d.addOn.goToStart()
        }
    }

    ActionItem {
        actionId: "core.timeline.goToPreviousMeasure"
        Action {
            onTriggered: d.addOn.goToPreviousMeasure()
        }
    }

    ActionItem {
        actionId: "core.timeline.goToPreviousBeat"
        Action {
            onTriggered: d.addOn.goToPreviousBeat()
        }
    }

    ActionItem {
        actionId: "core.timeline.goToPreviousTick"
        Action {
            onTriggered: d.addOn.goToPreviousTick()
        }
    }

    ActionItem {
        actionId: "core.timeline.goToEnd"
        Action {
            onTriggered: d.addOn.goToEnd()
        }
    }

    ActionItem {
        actionId: "core.timeline.goToNextMeasure"
        Action {
            onTriggered: d.addOn.goToNextMeasure()
        }
    }

    ActionItem {
        actionId: "core.timeline.goToNextBeat"
        Action {
            onTriggered: d.addOn.goToNextBeat()
        }
    }

    ActionItem {
        actionId: "core.timeline.goToNextTick"
        Action {
            onTriggered: d.addOn.goToNextTick()
        }
    }

    ActionItem {
        actionId: "core.timeline.resetProjectTimeRange"
        Action {
            onTriggered: d.addOn.resetProjectTimeRange()
        }
    }


}