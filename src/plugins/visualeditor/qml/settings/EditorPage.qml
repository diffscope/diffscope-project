// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

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
    property double autoDurationPositionAlignment: 32
    property bool enableTemporarySnapOff: false
    property bool trackListOnRight: false
    property bool centerPianoRollOnClipDoubleClick: true
    property bool pianoRollAutoPageVerticalAdjustment: false
    property int pianoRollAutoPageOverflowBehavior: EditorPreference.APOB_AlignTop
    property double pianoKeyboardBlackKeyLengthRatio: 0.6
    property int pianoKeyboardLabelPolicy: 0
    property int noteEditPitchCurveDisplayMode: EditorPreference.PCDM_Automatic
    property double autoPitchCurveDisplayPixelDensityThreshold: defaultAutoPitchCurveDisplayPixelDensityThreshold
    property bool displayPronunciationBelowNote: false
    property int shortNoteThreshold: 30
    property bool warnOfOverlappingNotes: true
    property bool trackCursorPosition: false
    property bool showSingerBackground: true
    property double singerBackgroundOpacity: 0.25
    property int singerBackgroundAlignment: EditorPreference.SBA_Right
    property int singerBackgroundHorizontalMargin: 32
    property int singerBackgroundHorizontalOffset: 0
    property int singerBackgroundVerticalMargin: 32
    property bool showAllSingerBackgrounds: true
    property int singerBackgroundSpacing: 16
    property double bottomPanelOpacity: 0.5

    onAlternateAxisModifierChanged: if (started) pageHandle.markDirty()
    onZoomModifierChanged: if (started) pageHandle.markDirty()
    onPageModifierChanged: if (started) pageHandle.markDirty()
    onUsePageModifierAsAlternateAxisZoomChanged: if (started) pageHandle.markDirty()
    onMiddleButtonAutoScrollChanged: if (started) pageHandle.markDirty()
    onAutoDurationPositionAlignmentChanged: if (started) pageHandle.markDirty()
    onEnableTemporarySnapOffChanged: if (started) pageHandle.markDirty()
    onTrackListOnRightChanged: if (started) pageHandle.markDirty()
    onCenterPianoRollOnClipDoubleClickChanged: if (started) pageHandle.markDirty()
    onPianoRollAutoPageVerticalAdjustmentChanged: if (started) pageHandle.markDirty()
    onPianoRollAutoPageOverflowBehaviorChanged: if (started) pageHandle.markDirty()
    onPianoKeyboardBlackKeyLengthRatioChanged: if (started) pageHandle.markDirty()
    onPianoKeyboardLabelPolicyChanged: if (started) pageHandle.markDirty()
    onNoteEditPitchCurveDisplayModeChanged: if (started) pageHandle.markDirty()
    onAutoPitchCurveDisplayPixelDensityThresholdChanged: if (started) pageHandle.markDirty()
    onDisplayPronunciationBelowNoteChanged: if (started) pageHandle.markDirty()
    onShortNoteThresholdChanged: if (started) pageHandle.markDirty()
    onWarnOfOverlappingNotesChanged: if (started) pageHandle.markDirty()
    onTrackCursorPositionChanged: if (started) pageHandle.markDirty()
    onShowSingerBackgroundChanged: if (started) pageHandle.markDirty()
    onSingerBackgroundOpacityChanged: if (started) pageHandle.markDirty()
    onSingerBackgroundAlignmentChanged: if (started) pageHandle.markDirty()
    onSingerBackgroundHorizontalMarginChanged: if (started) pageHandle.markDirty()
    onSingerBackgroundHorizontalOffsetChanged: if (started) pageHandle.markDirty()
    onSingerBackgroundVerticalMarginChanged: if (started) pageHandle.markDirty()
    onShowAllSingerBackgroundsChanged: if (started) pageHandle.markDirty()
    onSingerBackgroundSpacingChanged: if (started) pageHandle.markDirty()
    onBottomPanelOpacityChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    contentWidth: availableWidth

    readonly property TextMatcher matcher: TextMatcher {}
    readonly property double minAutoPitchCurveDisplayPixelDensityThreshold: 0.05
    readonly property double maxAutoPitchCurveDisplayPixelDensityThreshold: 0.1
    readonly property double defaultAutoPitchCurveDisplayPixelDensityThreshold:
        Math.sqrt(minAutoPitchCurveDisplayPixelDensityThreshold * maxAutoPitchCurveDisplayPixelDensityThreshold)

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
                        model: EditorInteractionHelper.scrollModifierTexts
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
                        model: EditorInteractionHelper.scrollModifierTexts
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
                        model: EditorInteractionHelper.scrollModifierTexts
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
                    columns: 2

                    Label {
                        text: qsTr("Auto-snap length")
                        TextMatcherItem on text {
                            matcher: page.matcher
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("More compact")
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 0.9 - Math.log10(32)
                            to: 1.9 - Math.log10(32)
                            ThemedItem.sliderTrackStartType: SVS.TS_Begin
                            value: Math.log10(page.autoDurationPositionAlignment) - Math.log10(32)
                            onMoved: page.autoDurationPositionAlignment = Math.pow(10, value +  Math.log10(32))
                            ThemedItem.onDoubleClickReset: moved()
                        }
                        Label {
                            text: qsTr("More spacious")
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                        }
                    }

                    CheckBox {
                        text: qsTr("Temporarily disable snap when pressing %1").arg(EditorInteractionHelper.shiftText)
                        Layout.columnSpan: 2
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

                    CheckBox {
                        text: qsTr("Center piano roll when double-clicking a singing clip")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 3
                        checked: page.centerPianoRollOnClipDoubleClick
                        onClicked: page.centerPianoRollOnClipDoubleClick = checked
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

                    CheckBox {
                        text: qsTr("Adjust vertical view during automatic page scrolling")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 3
                        checked: page.pianoRollAutoPageVerticalAdjustment
                        onClicked: page.pianoRollAutoPageVerticalAdjustment = checked
                    }
                    RowLayout {
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        Layout.leftMargin: autoPageOverflowComboBox.mirrored ? 0 : 22
                        Layout.rightMargin: autoPageOverflowComboBox.mirrored ? 22 : 0
                        enabled: page.pianoRollAutoPageVerticalAdjustment

                        Label {
                            id: autoPageOverflowLabel
                            text: qsTr("If notes exceed the view height")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        ComboBox {
                            id: autoPageOverflowComboBox
                            model: [qsTr("Align Top"), qsTr("Align Bottom"), qsTr("Center"), qsTr("Zoom to Fit")]
                            currentIndex: page.pianoRollAutoPageOverflowBehavior
                            Accessible.name: autoPageOverflowLabel.text
                            onActivated: (index) => page.pianoRollAutoPageOverflowBehavior = index
                        }
                    }

                    Label {
                        text: qsTr("Black key width")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Slider {
                        Layout.fillWidth: true
                        Layout.columnSpan: 2
                        from: -0.1
                        to: 0.4
                        ThemedItem.sliderTrackStartType: SVS.TS_Begin
                        value: page.pianoKeyboardBlackKeyLengthRatio - 0.6
                        onMoved: page.pianoKeyboardBlackKeyLengthRatio = value + 0.6
                        ThemedItem.onDoubleClickReset: moved()
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

                    Label {
                        text: qsTr("Pronunciation display position")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        model: [qsTr("Above note"), qsTr("Below note")]
                        currentIndex: page.displayPronunciationBelowNote ? 1 : 0
                        onActivated: (index) => page.displayPronunciationBelowNote = (index === 1)
                    }

                    Label {
                        text: qsTr("Pitch curve display in note edit mode")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        model: [qsTr("Automatic"), qsTr("Always show"), qsTr("Always hide")]
                        currentIndex: page.noteEditPitchCurveDisplayMode
                        onActivated: (index) => page.noteEditPitchCurveDisplayMode = index
                    }

                    RowLayout {
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        Layout.leftMargin: 22
                        enabled: page.noteEditPitchCurveDisplayMode === EditorPreference.PCDM_Automatic

                        Label {
                            text: qsTr("Automatic display zoom threshold")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: Math.log2(page.minAutoPitchCurveDisplayPixelDensityThreshold)
                            to: Math.log2(page.maxAutoPitchCurveDisplayPixelDensityThreshold)
                            value: Math.log2(page.autoPitchCurveDisplayPixelDensityThreshold)
                            onMoved: page.autoPitchCurveDisplayPixelDensityThreshold = Math.pow(2, value)
                            ThemedItem.sliderTrackStartType: SVS.TS_Begin
                            ThemedItem.doubleClickResetValue: Math.log2(page.defaultAutoPitchCurveDisplayPixelDensityThreshold)
                            ThemedItem.onDoubleClickReset: moved()
                        }
                    }

                    Label {
                        text: qsTr("Short note threshold")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        from: 1
                        to: 60
                        value: page.shortNoteThreshold
                        onValueModified: page.shortNoteThreshold = value
                    }

                    CheckBox {
                        text: qsTr("Warn of overlapping notes")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 3
                        checked: page.warnOfOverlappingNotes
                        onClicked: page.warnOfOverlappingNotes = checked
                    }

                    CheckBox {
                        text: qsTr("Show singer background")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 3
                        checked: page.showSingerBackground
                        onClicked: page.showSingerBackground = checked
                    }

                    RowLayout {
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        Layout.leftMargin: singerBackgroundOpacitySpinBox.mirrored ? 0 : 22
                        Layout.rightMargin: singerBackgroundOpacitySpinBox.mirrored ? 22 : 0
                        enabled: page.showSingerBackground

                        Label {
                            id: singerBackgroundOpacityLabel
                            text: qsTr("Singer background opacity")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: 1
                            value: page.singerBackgroundOpacity
                            Accessible.name: singerBackgroundOpacityLabel.text
                            onMoved: page.singerBackgroundOpacity = value
                            ThemedItem.sliderTrackStartType: SVS.TS_Begin
                            ThemedItem.onDoubleClickReset: page.singerBackgroundOpacity = 0.25
                        }
                        DoubleSpinBox {
                            id: singerBackgroundOpacitySpinBox
                            decimals: 0
                            from: 0
                            to: 100
                            value: Math.round(page.singerBackgroundOpacity * 100)
                            Accessible.name: singerBackgroundOpacityLabel.text
                            onValueModified: page.singerBackgroundOpacity = value / 100.0
                            textFromValue: function(value, decimals, locale) {
                                return value + "%"
                            }
                            valueFromText: function(text, locale) {
                                return parseInt(text.replace("%", ""))
                            }
                        }
                    }

                    RowLayout {
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        Layout.leftMargin: singerBackgroundAlignmentComboBox.mirrored ? 0 : 22
                        Layout.rightMargin: singerBackgroundAlignmentComboBox.mirrored ? 22 : 0
                        enabled: page.showSingerBackground

                        Label {
                            id: singerBackgroundAlignmentLabel
                            text: qsTr("Background alignment")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        ComboBox {
                            id: singerBackgroundAlignmentComboBox
                            model: [qsTr("Left"), qsTr("Center"), qsTr("Right")]
                            currentIndex: page.singerBackgroundAlignment
                            Accessible.name: singerBackgroundAlignmentLabel.text
                            onActivated: (index) => page.singerBackgroundAlignment = index
                        }
                    }

                    RowLayout {
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        Layout.leftMargin: singerBackgroundHorizontalMarginSpinBox.mirrored ? 0 : 22
                        Layout.rightMargin: singerBackgroundHorizontalMarginSpinBox.mirrored ? 22 : 0
                        enabled: page.showSingerBackground
                                 && (page.singerBackgroundAlignment === EditorPreference.SBA_Left
                                     || page.singerBackgroundAlignment === EditorPreference.SBA_Right)

                        Label {
                            id: singerBackgroundHorizontalMarginLabel
                            text: qsTr("Horizontal margin")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: 200
                            stepSize: 1
                            snapMode: Slider.SnapAlways
                            value: page.singerBackgroundHorizontalMargin
                            Accessible.name: singerBackgroundHorizontalMarginLabel.text
                            onMoved: page.singerBackgroundHorizontalMargin = Math.round(value)
                            ThemedItem.sliderTrackStartType: SVS.TS_Begin
                            ThemedItem.onDoubleClickReset: page.singerBackgroundHorizontalMargin = 32
                        }
                        SpinBox {
                            id: singerBackgroundHorizontalMarginSpinBox
                            from: 0
                            to: 200
                            value: page.singerBackgroundHorizontalMargin
                            Accessible.name: singerBackgroundHorizontalMarginLabel.text
                            onValueModified: page.singerBackgroundHorizontalMargin = value
                        }
                    }

                    RowLayout {
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        Layout.leftMargin: singerBackgroundHorizontalOffsetSpinBox.mirrored ? 0 : 22
                        Layout.rightMargin: singerBackgroundHorizontalOffsetSpinBox.mirrored ? 22 : 0
                        enabled: page.showSingerBackground
                                 && page.singerBackgroundAlignment === EditorPreference.SBA_Center

                        Label {
                            id: singerBackgroundHorizontalOffsetLabel
                            text: qsTr("Horizontal offset")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: -200
                            to: 200
                            stepSize: 1
                            snapMode: Slider.SnapAlways
                            value: page.singerBackgroundHorizontalOffset
                            Accessible.name: singerBackgroundHorizontalOffsetLabel.text
                            onMoved: page.singerBackgroundHorizontalOffset = Math.round(value)
                            ThemedItem.onDoubleClickReset: page.singerBackgroundHorizontalOffset = 0
                        }
                        SpinBox {
                            id: singerBackgroundHorizontalOffsetSpinBox
                            from: -200
                            to: 200
                            value: page.singerBackgroundHorizontalOffset
                            Accessible.name: singerBackgroundHorizontalOffsetLabel.text
                            onValueModified: page.singerBackgroundHorizontalOffset = value
                        }
                    }

                    RowLayout {
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        Layout.leftMargin: singerBackgroundVerticalMarginSpinBox.mirrored ? 0 : 22
                        Layout.rightMargin: singerBackgroundVerticalMarginSpinBox.mirrored ? 22 : 0
                        enabled: page.showSingerBackground

                        Label {
                            id: singerBackgroundVerticalMarginLabel
                            text: qsTr("Vertical margin")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: 100
                            stepSize: 1
                            snapMode: Slider.SnapAlways
                            value: page.singerBackgroundVerticalMargin
                            Accessible.name: singerBackgroundVerticalMarginLabel.text
                            onMoved: page.singerBackgroundVerticalMargin = Math.round(value)
                            ThemedItem.sliderTrackStartType: SVS.TS_Begin
                            ThemedItem.onDoubleClickReset: page.singerBackgroundVerticalMargin = 32
                        }
                        SpinBox {
                            id: singerBackgroundVerticalMarginSpinBox
                            from: 0
                            to: 100
                            value: page.singerBackgroundVerticalMargin
                            Accessible.name: singerBackgroundVerticalMarginLabel.text
                            onValueModified: page.singerBackgroundVerticalMargin = value
                        }
                    }

                    CheckBox {
                        id: showAllSingerBackgroundsCheckBox
                        text: qsTr("Show all singer backgrounds when blending voices")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 3
                        Layout.leftMargin: mirrored ? 0 : 22
                        Layout.rightMargin: mirrored ? 22 : 0
                        enabled: page.showSingerBackground
                        checked: page.showAllSingerBackgrounds
                        onClicked: page.showAllSingerBackgrounds = checked
                    }

                    RowLayout {
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        Layout.leftMargin: singerBackgroundSpacingSpinBox.mirrored ? 0 : 44
                        Layout.rightMargin: singerBackgroundSpacingSpinBox.mirrored ? 44 : 0
                        enabled: page.showSingerBackground && page.showAllSingerBackgrounds

                        Label {
                            id: singerBackgroundSpacingLabel
                            text: qsTr("Singer background spacing")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: 100
                            stepSize: 1
                            snapMode: Slider.SnapAlways
                            value: page.singerBackgroundSpacing
                            Accessible.name: singerBackgroundSpacingLabel.text
                            onMoved: page.singerBackgroundSpacing = Math.round(value)
                            ThemedItem.sliderTrackStartType: SVS.TS_Begin
                            ThemedItem.onDoubleClickReset: page.singerBackgroundSpacing = 16
                        }
                        SpinBox {
                            id: singerBackgroundSpacingSpinBox
                            from: 0
                            to: 100
                            value: page.singerBackgroundSpacing
                            Accessible.name: singerBackgroundSpacingLabel.text
                            onValueModified: page.singerBackgroundSpacing = value
                        }
                    }

                    Label {
                        text: qsTr("Bottom panel opacity")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Slider {
                        Layout.fillWidth: true
                        from: 0
                        to: 1
                        value: page.bottomPanelOpacity
                        onMoved: page.bottomPanelOpacity = value
                        ThemedItem.sliderTrackStartType: SVS.TS_Begin
                        ThemedItem.onDoubleClickReset: page.bottomPanelOpacity = 0.5
                    }
                    DoubleSpinBox {
                        decimals: 0
                        from: 0
                        to: 100
                        value: Math.round(page.bottomPanelOpacity * 100)
                        onValueModified: page.bottomPanelOpacity = value / 100.0
                        textFromValue: function(value, decimals, locale) {
                            return value + "%"
                        }
                        valueFromText: function(text, locale) {
                            return parseInt(text.replace("%", ""))
                        }
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
