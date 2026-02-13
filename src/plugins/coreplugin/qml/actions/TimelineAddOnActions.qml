import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.UIShell
import DiffScope.Core

ActionCollection {
    id: d

    required property TimelineAddOn addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null
    readonly property Window window: addOn?.windowHandle.window ?? null
    readonly property EditTempoTimeSignatureScenario editTempoTimeSignatureScenario: EditTempoTimeSignatureScenario {
        id: editTempoTimeSignatureScenario
        window: d.window
        projectTimeline: d.windowHandle?.projectTimeline ?? null
        document: d.windowHandle?.projectDocumentContext.document ?? null
    }
    readonly property EditLoopScenario editLoopScenario: EditLoopScenario {
        id: editLoopScenario
        window: d.window
        projectTimeline: windowHandle?.projectTimeline ?? null
        document: windowHandle?.projectDocumentContext.document ?? null
    }

    ActionItem {
        actionId: "org.diffscope.core.widget.digitalClock"
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
                    d.windowHandle.triggerAction("org.diffscope.core.timeline.goTo", this)
                } else if (flag === BehaviorPreference.TIIB_ShowQuickJump) {
                    d.addOn.execQuickJump(text)
                }
            }
            Menu {
                id: digitalClockContextMenu
                MenuActionInstantiator {
                    actionId: "org.diffscope.core.timeline"
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
        actionId: "org.diffscope.core.widget.tempoTimeSignatureIndicator"
        TempoTimeSignatureIndicator {
            id: tempoTimeSignatureIndicator

            backgroundVisible: BehaviorPreference.timeIndicatorBackgroundVisible
            tempoText: d.addOn.tempoText
            timeSignatureText: d.addOn.timeSignatureText

            onTempoClicked: editTempoTimeSignatureScenario.editTempo()
            onTimeSignatureClicked: editTempoTimeSignatureScenario.editTimeSignature()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeIndicator.showMusicTime"
        Action {
            checkable: true
            checked: d.addOn.showMusicTime
            onTriggered: d.addOn.showMusicTime = checked
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeIndicator.showAbsoluteTime"
        Action {
            checkable: true
            checked: d.addOn.showAbsoluteTime
            onTriggered: d.addOn.showAbsoluteTime = checked
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.goTo"
        Action {

        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.quickJump"
        Action {
            onTriggered: Qt.callLater(() => d.addOn.execQuickJump())
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.goToStart"
        Action {
            onTriggered: d.addOn.goToStart()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.goToPreviousMeasure"
        Action {
            onTriggered: d.addOn.goToPreviousMeasure()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.goToPreviousBeat"
        Action {
            onTriggered: d.addOn.goToPreviousBeat()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.goToPreviousTick"
        Action {
            onTriggered: d.addOn.goToPreviousTick()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.goToEnd"
        Action {
            onTriggered: d.addOn.goToEnd()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.goToNextMeasure"
        Action {
            onTriggered: d.addOn.goToNextMeasure()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.goToNextBeat"
        Action {
            onTriggered: d.addOn.goToNextBeat()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.goToNextTick"
        Action {
            onTriggered: d.addOn.goToNextTick()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.resetProjectTimeRange"
        Action {
            onTriggered: d.addOn.resetProjectTimeRange()
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.editTempo"
        Action {
            onTriggered: Qt.callLater(() => editTempoTimeSignatureScenario.editTempo())
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.editTimeSignature"
        Action {
            onTriggered: Qt.callLater(() => editTempoTimeSignatureScenario.editTimeSignature())
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.editLoop"
        Action {
            onTriggered: Qt.callLater(() => editLoopScenario.editLoop())
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.enableLoop"
        Action {
            checkable: true
            checked: d.addOn.loopEnabled
            onTriggered: d.addOn.loopEnabled = checked
        }
    }


}