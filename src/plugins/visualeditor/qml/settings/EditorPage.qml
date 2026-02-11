import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.VisualEditor

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false
    property int alternateAxisModifier: 0
    property int zoomModifier: 0
    property int pageModifier: 0
    property bool usePageModifierAsAlternateAxisZoom: false
    property bool middleButtonAutoScroll: false
    property int autoDurationPositionAlignment: 20
    property bool enableTemporarySnapOff: false
    property bool trackListOnRight: false
    property bool pianoKeyboardUseSimpleStyle: false
    property int pianoKeyboardLabelPolicy: 0
    property bool trackCursorPosition: false

    onAlternateAxisModifierChanged: if (started) pageHandle.markDirty()
    onZoomModifierChanged: if (started) pageHandle.markDirty()
    onPageModifierChanged: if (started) pageHandle.markDirty()
    onUsePageModifierAsAlternateAxisZoomChanged: if (started) pageHandle.markDirty()
    onMiddleButtonAutoScrollChanged: if (started) pageHandle.markDirty()
    onAutoDurationPositionAlignmentChanged: if (started) pageHandle.markDirty()
    onEnableTemporarySnapOffChanged: if (started) pageHandle.markDirty()
    onTrackListOnRightChanged: if (started) pageHandle.markDirty()
    onPianoKeyboardUseSimpleStyleChanged: if (started) pageHandle.markDirty()
    onPianoKeyboardLabelPolicyChanged: if (started) pageHandle.markDirty()
    onTrackCursorPositionChanged: if (started) pageHandle.markDirty()

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
                title: qsTr("Move and Zoom")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                GridLayout {
                    anchors.fill: parent
                    columns: 3

                    Label {
                        text: qsTr("Horizontal scroll")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        model: page.pageHandle.scrollModifierTexts
                        currentIndex: page.alternateAxisModifier
                        onActivated: (index) => page.alternateAxisModifier = index
                    }

                    Label {
                        text: qsTr("Zoom")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        model: page.pageHandle.scrollModifierTexts
                        currentIndex: page.zoomModifier
                        onActivated: (index) => page.zoomModifier = index
                    }

                    RowLayout {
                        RadioButton {
                            text: qsTr("Scroll by page")
                            TextMatcherItem on text { matcher: page.matcher }
                            checked: !page.usePageModifierAsAlternateAxisZoom
                            onClicked: page.usePageModifierAsAlternateAxisZoom = false
                        }
                        RadioButton {
                            text: qsTr("Horizontal zoom")
                            TextMatcherItem on text { matcher: page.matcher }
                            checked: page.usePageModifierAsAlternateAxisZoom
                            onClicked: page.usePageModifierAsAlternateAxisZoom = true
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        model: page.pageHandle.scrollModifierTexts
                        currentIndex: page.pageModifier
                        onActivated: (index) => page.pageModifier = index
                    }
                    Label {
                        text: qsTr("Middle button/hand tool scroll mode")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        model: [qsTr("Dragging"), qsTr("Auto scrolling")]
                        currentIndex: page.middleButtonAutoScroll ? 1 : 0
                        onActivated: (index) => page.middleButtonAutoScroll = (index === 1)
                    }
                }
            }

            GroupBox {
                title: qsTr("Snap")
                TextMatcherItem on title {
                    matcher: page.matcher
                }
                Layout.fillWidth: true

                GridLayout {
                    anchors.fill: parent
                    columns: 3

                    Label {
                        text: qsTr("Auto-snap length")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }
                    Slider {
                        Layout.fillWidth: true
                        from: 0
                        to: 80

                        stepSize: 1
                        snapMode: Slider.SnapAlways
                        value: page.autoDurationPositionAlignment - 20
                        onMoved: page.autoDurationPositionAlignment = value + 20
                    }
                    SpinBox {
                        from: 20
                        to: 100
                        stepSize: 1
                        value: page.autoDurationPositionAlignment
                        onValueModified: page.autoDurationPositionAlignment = value
                    }

                    CheckBox {
                        text: qsTr("Temporarily disable snap when pressing %1").arg(page.pageHandle.shiftText)
                        Layout.columnSpan: 3
                        checked: page.enableTemporarySnapOff
                        onClicked: page.enableTemporarySnapOff = checked
                    }

                }
            }

            GroupBox {
                title: qsTr("Arrangement")
                TextMatcherItem on title {
                    matcher: page.matcher
                }
                Layout.fillWidth: true

                GridLayout {
                    anchors.fill: parent
                    columns: 3

                    Label {
                        text: qsTr("Track list position")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        model: [qsTr("Left"), qsTr("Right")]
                        currentIndex: page.trackListOnRight ? 1 : 0
                        onActivated: (index) => page.trackListOnRight = (index === 1)
                    }
                }
            }

            GroupBox {
                title: qsTr("Piano Roll")
                TextMatcherItem on title {
                    matcher: page.matcher
                }
                Layout.fillWidth: true

                GridLayout {
                    anchors.fill: parent
                    columns: 3

                    Label {
                        text: qsTr("Piano keyboard style")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        model: [qsTr("Realistic"), qsTr("Simple")]
                        currentIndex: page.pianoKeyboardUseSimpleStyle ? 1 : 0
                        onActivated: (index) => page.pianoKeyboardUseSimpleStyle = (index === 1)
                    }

                    Label {
                        text: qsTr("Display piano key label on")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        model: [qsTr("All keys"), qsTr("C keys only"), qsTr("None")]
                        currentIndex: page.pianoKeyboardLabelPolicy
                        onActivated: (index) => page.pianoKeyboardLabelPolicy = index
                    }
                }
            }

            GroupBox {
                title: qsTr("Miscellaneous")
                TextMatcherItem on title {
                    matcher: page.matcher
                }
                Layout.fillWidth: true

                GridLayout {
                    anchors.fill: parent
                    columns: 3

                    CheckBox {
                        text: qsTr("Track cursor position")
                        Layout.columnSpan: 3
                        checked: page.trackCursorPosition
                        onClicked: page.trackCursorPosition = checked
                    }

                }
            }

        }
    }
}