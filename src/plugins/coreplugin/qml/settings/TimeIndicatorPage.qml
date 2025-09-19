import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false

    property bool timeIndicatorBackgroundVisible
    property int timeIndicatorClickAction
    property int timeIndicatorDoubleClickAction
    property int timeIndicatorPressAndHoldAction
    property bool timeIndicatorTextFineTuneEnabled
    property bool timeIndicatorShowSliderOnHover

    readonly property var interactionBehaviorModel: [
        {text: qsTr("Toggle timecode format"), value: BehaviorPreference.TIIB_ToggleFormat},
        {text: qsTr('Show "Go To"'), value: BehaviorPreference.TIIB_ShowGoTo},
        {text: qsTr('Show "Quick Jump"'), value: BehaviorPreference.TIIB_ShowQuickJump},
    ]

    onTimeIndicatorBackgroundVisibleChanged: if (started) pageHandle.markDirty()
    onTimeIndicatorClickActionChanged: if (started) pageHandle.markDirty()
    onTimeIndicatorDoubleClickActionChanged: if (started) pageHandle.markDirty()
    onTimeIndicatorPressAndHoldActionChanged: if (started) pageHandle.markDirty()
    onTimeIndicatorTextFineTuneEnabledChanged: if (started) pageHandle.markDirty()
    onTimeIndicatorShowSliderOnHoverChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    contentWidth: availableWidth

    readonly property TextMatcher matcher: TextMatcher {}

    ColumnLayout {
        width: page.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32
            GroupBox {
                title: qsTr("Display")
                TextMatcherItem on title {
                    matcher: page.matcher
                }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Show background")
                        checked: page.timeIndicatorBackgroundVisible
                        onClicked: page.timeIndicatorBackgroundVisible = checked
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    CheckBox {
                        text: qsTr("Fine-tune character spacing")
                        checked: page.timeIndicatorTextFineTuneEnabled
                        onClicked: page.timeIndicatorTextFineTuneEnabled = checked
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    Label {
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("Enabling fine-tuning of character spacing can prevent text width changes caused by timecode changes")
                    }
                }
            }
            GroupBox {
                title: qsTr("Interaction Behavior")
                TextMatcherItem on title {
                    matcher: page.matcher
                }
                Layout.fillWidth: true
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    CheckBox {
                        Layout.columnSpan: 2
                        text: qsTr("Show slider on hover")
                        checked: page.timeIndicatorShowSliderOnHover
                        onClicked: page.timeIndicatorShowSliderOnHover = checked
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    Label {
                        text: qsTr("Click action")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    ComboBox {
                        textRole: "text"
                        valueRole: "value"
                        model: page.interactionBehaviorModel
                        implicitContentWidthPolicy: ComboBox.WidestText
                        currentIndex: page.timeIndicatorClickAction
                        onCurrentValueChanged: page.timeIndicatorClickAction = currentValue
                    }
                    Label {
                        text: qsTr("Double-click action")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    ComboBox {
                        textRole: "text"
                        valueRole: "value"
                        model: page.interactionBehaviorModel
                        implicitContentWidthPolicy: ComboBox.WidestText
                        currentIndex: page.timeIndicatorDoubleClickAction
                        onCurrentValueChanged: page.timeIndicatorDoubleClickAction = currentValue
                    }
                    Label {
                        text: qsTr("Press-and-hold action")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    ComboBox {
                        textRole: "text"
                        valueRole: "value"
                        model: page.interactionBehaviorModel
                        implicitContentWidthPolicy: ComboBox.WidestText
                        currentIndex: page.timeIndicatorPressAndHoldAction
                        onCurrentValueChanged: page.timeIndicatorPressAndHoldAction = currentValue
                    }
                }
            }
        }
    }
}
