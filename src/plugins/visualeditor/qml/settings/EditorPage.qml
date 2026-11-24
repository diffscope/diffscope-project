// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import dev.sjimo.ScopicFlow

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
    property int preferredParameterShiftMode: ParameterEditorInteractionController.TranslateValues
    property int preferredParameterScaleMode: ParameterEditorInteractionController.ScaleToEndpoints
    property double autoDurationPositionAlignment: 32
    property bool enableTemporarySnapOff: false
    property int autoPageScrollRetainedArea
    property int autoPageScrollTriggerArea
    property int autoPageScrollAnimationDuration
    property bool autoPageScrollTriggerOutsidePlayback
    property bool trackListOnRight: false
    property bool centerPianoRollOnClipDoubleClick: true
    property bool pianoRollAutoPageVerticalAdjustment: false
    property int pianoRollAutoPageOverflowBehavior: EditorPreference.APOB_AlignTop
    property double pianoKeyboardBlackKeyLengthRatio: 0.6
    property int pianoKeyboardLabelPolicy: 0
    property int noteEditPitchCurveDisplayMode: EditorPreference.PCDM_Automatic
    property double autoPitchCurveDisplayPixelDensityThreshold: defaultAutoPitchCurveDisplayPixelDensityThreshold
    property bool displayPronunciationBelowNote: false
    property bool showPhonemeNoteBoundaryOverlay: true
    property int parameterPanelOverlayMode: EditorPreference.POM_FirstPhoneme
    property bool showLyricOverlay: false
    property int lyricPhraseGapThreshold: 240
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
    onPreferredParameterShiftModeChanged: if (started) pageHandle.markDirty()
    onPreferredParameterScaleModeChanged: if (started) pageHandle.markDirty()
    onAutoDurationPositionAlignmentChanged: if (started) pageHandle.markDirty()
    onEnableTemporarySnapOffChanged: if (started) pageHandle.markDirty()
    onAutoPageScrollRetainedAreaChanged: if (started) pageHandle.markDirty()
    onAutoPageScrollTriggerAreaChanged: if (started) pageHandle.markDirty()
    onAutoPageScrollAnimationDurationChanged: if (started) pageHandle.markDirty()
    onAutoPageScrollTriggerOutsidePlaybackChanged: if (started) pageHandle.markDirty()
    onTrackListOnRightChanged: if (started) pageHandle.markDirty()
    onCenterPianoRollOnClipDoubleClickChanged: if (started) pageHandle.markDirty()
    onPianoRollAutoPageVerticalAdjustmentChanged: if (started) pageHandle.markDirty()
    onPianoRollAutoPageOverflowBehaviorChanged: if (started) pageHandle.markDirty()
    onPianoKeyboardBlackKeyLengthRatioChanged: if (started) pageHandle.markDirty()
    onPianoKeyboardLabelPolicyChanged: if (started) pageHandle.markDirty()
    onNoteEditPitchCurveDisplayModeChanged: if (started) pageHandle.markDirty()
    onAutoPitchCurveDisplayPixelDensityThresholdChanged: if (started) pageHandle.markDirty()
    onDisplayPronunciationBelowNoteChanged: if (started) pageHandle.markDirty()
    onShowPhonemeNoteBoundaryOverlayChanged: if (started) pageHandle.markDirty()
    onParameterPanelOverlayModeChanged: if (started) pageHandle.markDirty()
    onShowLyricOverlayChanged: if (started) pageHandle.markDirty()
    onLyricPhraseGapThresholdChanged: if (started) pageHandle.markDirty()
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

    component PageScrollSetting: RowLayout {
        id: setting

        required property string label
        required property int maximum
        required property int value
        required property int defaultValue
        signal valueModified(int newValue)

        Layout.fillWidth: true

        Slider {
            Layout.fillWidth: true
            to: setting.maximum
            value: setting.value
            stepSize: 1
            snapMode: Slider.SnapAlways
            Accessible.name: setting.label
            onMoved: setting.valueModified(value)
            ThemedItem.sliderTrackStartType: SVS.TS_Begin
            ThemedItem.onDoubleClickReset: setting.valueModified(setting.defaultValue)
        }
        SpinBox {
            to: setting.maximum
            value: setting.value
            Accessible.name: setting.label
            onValueModified: setting.valueModified(value)
        }
    }

    function selectScrollModifier(propertyName: string, modifier: int) {
        const previousModifier = page[propertyName]
        if (previousModifier === modifier)
            return
        for (const otherProperty of ["alternateAxisModifier", "zoomModifier", "pageModifier"]) {
            if (otherProperty !== propertyName && page[otherProperty] === modifier) {
                page[otherProperty] = previousModifier
                break
            }
        }
        page[propertyName] = modifier
    }

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
                        onActivated: (index) => page.selectScrollModifier("alternateAxisModifier", index)
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
                        onActivated: (index) => page.selectScrollModifier("zoomModifier", index)
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
                        onActivated: (index) => page.selectScrollModifier("pageModifier", index)
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
                title: qsTr("Auto Page Scrolling")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                GridLayout {
                    anchors.fill: parent
                    columns: 2

                    Label {
                        id: retainedAreaLabel
                        text: qsTr("Retained area (%)")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    PageScrollSetting {
                        label: retainedAreaLabel.text
                        maximum: 25
                        value: page.autoPageScrollRetainedArea
                        defaultValue: 10
                        onValueModified: (newValue) => page.autoPageScrollRetainedArea = newValue
                    }

                    Label {
                        id: triggerAreaLabel
                        text: qsTr("Trigger area (%)")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    PageScrollSetting {
                        label: triggerAreaLabel.text
                        maximum: 25
                        value: page.autoPageScrollTriggerArea
                        defaultValue: 10
                        onValueModified: (newValue) => page.autoPageScrollTriggerArea = newValue
                    }

                    Label {
                        id: animationDurationLabel
                        text: qsTr("Animation duration (ms)")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    PageScrollSetting {
                        label: animationDurationLabel.text
                        maximum: 1000
                        value: page.autoPageScrollAnimationDuration
                        defaultValue: 250
                        onValueModified: (newValue) => page.autoPageScrollAnimationDuration = newValue
                    }

                    CheckBox {
                        text: qsTr("Use trigger area outside playback")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 2
                        checked: page.autoPageScrollTriggerOutsidePlayback
                        onClicked: page.autoPageScrollTriggerOutsidePlayback = checked
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
                    id: pianoRollLayout

                    readonly property int subOptionIndent: 22

                    anchors.fill: parent
                    columns: 3
                    columnSpacing: 12
                    rowSpacing: 8

                    CheckBox {
                        text: qsTr("Adjust vertical view during automatic page scrolling")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 3
                        checked: page.pianoRollAutoPageVerticalAdjustment
                        onClicked: page.pianoRollAutoPageVerticalAdjustment = checked
                    }

                    Label {
                        id: autoPageOverflowLabel
                        enabled: page.pianoRollAutoPageVerticalAdjustment
                        Layout.leftMargin: mirrored ? 0 : pianoRollLayout.subOptionIndent
                        Layout.rightMargin: mirrored ? pianoRollLayout.subOptionIndent : 0
                        Layout.columnSpan: 2
                        text: qsTr("If notes exceed the view height")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        implicitContentWidthPolicy: ComboBox.WidestText
                        enabled: autoPageOverflowLabel.enabled
                        model: [qsTr("Align Top"), qsTr("Align Bottom"), qsTr("Center"), qsTr("Zoom to Fit")]
                        currentIndex: page.pianoRollAutoPageOverflowBehavior
                        Accessible.name: autoPageOverflowLabel.text
                        onActivated: (index) => page.pianoRollAutoPageOverflowBehavior = index
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
                        Layout.columnSpan: 2
                        text: qsTr("Display piano key label on")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        implicitContentWidthPolicy: ComboBox.WidestText
                        model: [qsTr("All keys"), qsTr("C keys only"), qsTr("None")]
                        currentIndex: page.pianoKeyboardLabelPolicy
                        onActivated: (index) => page.pianoKeyboardLabelPolicy = index
                    }

                    Label {
                        Layout.columnSpan: 2
                        text: qsTr("Pronunciation display position")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        implicitContentWidthPolicy: ComboBox.WidestText
                        model: [qsTr("Above note"), qsTr("Below note")]
                        currentIndex: page.displayPronunciationBelowNote ? 1 : 0
                        onActivated: (index) => page.displayPronunciationBelowNote = (index === 1)
                    }

                    Label {
                        Layout.columnSpan: 2
                        text: qsTr("Pitch curve display in note edit mode")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        implicitContentWidthPolicy: ComboBox.WidestText
                        model: [qsTr("Automatic"), qsTr("Always show"), qsTr("Always hide")]
                        currentIndex: page.noteEditPitchCurveDisplayMode
                        onActivated: (index) => page.noteEditPitchCurveDisplayMode = index
                    }

                    Label {
                        id: autoPitchCurveDisplayThresholdLabel
                        enabled: page.noteEditPitchCurveDisplayMode === EditorPreference.PCDM_Automatic
                        Layout.leftMargin: mirrored ? 0 : pianoRollLayout.subOptionIndent
                        Layout.rightMargin: mirrored ? pianoRollLayout.subOptionIndent : 0
                        text: qsTr("Automatic display zoom threshold")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Slider {
                        Layout.columnSpan: 2
                        enabled: autoPitchCurveDisplayThresholdLabel.enabled
                        Layout.fillWidth: true
                        from: Math.log2(page.minAutoPitchCurveDisplayPixelDensityThreshold)
                        to: Math.log2(page.maxAutoPitchCurveDisplayPixelDensityThreshold)
                        value: Math.log2(page.autoPitchCurveDisplayPixelDensityThreshold)
                        onMoved: page.autoPitchCurveDisplayPixelDensityThreshold = Math.pow(2, value)
                        ThemedItem.sliderTrackStartType: SVS.TS_Begin
                        ThemedItem.doubleClickResetValue: Math.log2(page.defaultAutoPitchCurveDisplayPixelDensityThreshold)
                        ThemedItem.onDoubleClickReset: moved()
                    }

                    CheckBox {
                        text: qsTr("Warn of overlapping notes")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 3
                        checked: page.warnOfOverlappingNotes
                        onClicked: page.warnOfOverlappingNotes = checked
                    }

                    CheckBox {
                        text: qsTr("Show lyric overlay")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 3
                        checked: page.showLyricOverlay
                        onClicked: page.showLyricOverlay = checked
                    }

                    Label {
                        id: lyricPhraseGapLabel
                        enabled: page.showLyricOverlay
                        Layout.leftMargin: mirrored ? 0 : pianoRollLayout.subOptionIndent
                        Layout.rightMargin: mirrored ? pianoRollLayout.subOptionIndent : 0
                        Layout.columnSpan: 2
                        text: qsTr("Lyric phrase gap threshold (ticks)")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    SpinBox {
                        Layout.fillWidth: true
                        enabled: lyricPhraseGapLabel.enabled
                        from: 1
                        to: 1920
                        value: page.lyricPhraseGapThreshold
                        Accessible.name: lyricPhraseGapLabel.text
                        onValueModified: page.lyricPhraseGapThreshold = value
                    }

                    CheckBox {
                        text: qsTr("Show singer background")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 3
                        checked: page.showSingerBackground
                        onClicked: page.showSingerBackground = checked
                    }

                    Label {
                        id: singerBackgroundOpacityLabel
                        enabled: page.showSingerBackground
                        Layout.leftMargin: mirrored ? 0 : pianoRollLayout.subOptionIndent
                        Layout.rightMargin: mirrored ? pianoRollLayout.subOptionIndent : 0
                        text: qsTr("Singer background opacity")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Slider {
                        enabled: singerBackgroundOpacityLabel.enabled
                        Layout.fillWidth: true
                        Layout.horizontalStretchFactor: 1
                        from: 0
                        to: 1
                        value: page.singerBackgroundOpacity
                        Accessible.name: singerBackgroundOpacityLabel.text
                        onMoved: page.singerBackgroundOpacity = value
                        ThemedItem.sliderTrackStartType: SVS.TS_Begin
                        ThemedItem.onDoubleClickReset: page.singerBackgroundOpacity = 0.25
                    }
                    DoubleSpinBox {
                        Layout.fillWidth: true
                        enabled: singerBackgroundOpacityLabel.enabled
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

                    Label {
                        id: singerBackgroundAlignmentLabel
                        enabled: page.showSingerBackground
                        Layout.leftMargin: mirrored ? 0 : pianoRollLayout.subOptionIndent
                        Layout.rightMargin: mirrored ? pianoRollLayout.subOptionIndent : 0
                        Layout.columnSpan: 2
                        text: qsTr("Background alignment")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        implicitContentWidthPolicy: ComboBox.WidestText
                        enabled: singerBackgroundAlignmentLabel.enabled
                        model: [qsTr("Left"), qsTr("Center"), qsTr("Right")]
                        currentIndex: page.singerBackgroundAlignment
                        Accessible.name: singerBackgroundAlignmentLabel.text
                        onActivated: (index) => page.singerBackgroundAlignment = index
                    }

                    Label {
                        id: singerBackgroundHorizontalMarginLabel
                        enabled: page.showSingerBackground
                                 && (page.singerBackgroundAlignment === EditorPreference.SBA_Left
                                     || page.singerBackgroundAlignment === EditorPreference.SBA_Right)
                        Layout.leftMargin: mirrored ? 0 : pianoRollLayout.subOptionIndent
                        Layout.rightMargin: mirrored ? pianoRollLayout.subOptionIndent : 0
                        text: qsTr("Horizontal margin")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Slider {
                        enabled: singerBackgroundHorizontalMarginLabel.enabled
                        Layout.fillWidth: true
                        Layout.horizontalStretchFactor: 1
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
                        Layout.fillWidth: true
                        enabled: singerBackgroundHorizontalMarginLabel.enabled
                        from: 0
                        to: 200
                        value: page.singerBackgroundHorizontalMargin
                        Accessible.name: singerBackgroundHorizontalMarginLabel.text
                        onValueModified: page.singerBackgroundHorizontalMargin = value
                    }

                    Label {
                        id: singerBackgroundHorizontalOffsetLabel
                        enabled: page.showSingerBackground
                                 && page.singerBackgroundAlignment === EditorPreference.SBA_Center
                        Layout.leftMargin: mirrored ? 0 : pianoRollLayout.subOptionIndent
                        Layout.rightMargin: mirrored ? pianoRollLayout.subOptionIndent : 0
                        text: qsTr("Horizontal offset")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Slider {
                        enabled: singerBackgroundHorizontalOffsetLabel.enabled
                        Layout.fillWidth: true
                        Layout.horizontalStretchFactor: 1
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
                        Layout.fillWidth: true
                        enabled: singerBackgroundHorizontalOffsetLabel.enabled
                        from: -200
                        to: 200
                        value: page.singerBackgroundHorizontalOffset
                        Accessible.name: singerBackgroundHorizontalOffsetLabel.text
                        onValueModified: page.singerBackgroundHorizontalOffset = value
                    }

                    Label {
                        id: singerBackgroundVerticalMarginLabel
                        enabled: page.showSingerBackground
                        Layout.leftMargin: mirrored ? 0 : pianoRollLayout.subOptionIndent
                        Layout.rightMargin: mirrored ? pianoRollLayout.subOptionIndent : 0
                        text: qsTr("Vertical margin")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Slider {
                        enabled: singerBackgroundVerticalMarginLabel.enabled
                        Layout.fillWidth: true
                        Layout.horizontalStretchFactor: 1
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
                        Layout.fillWidth: true
                        enabled: singerBackgroundVerticalMarginLabel.enabled
                        from: 0
                        to: 100
                        value: page.singerBackgroundVerticalMargin
                        Accessible.name: singerBackgroundVerticalMarginLabel.text
                        onValueModified: page.singerBackgroundVerticalMargin = value
                    }

                    CheckBox {
                        id: showAllSingerBackgroundsCheckBox
                        text: qsTr("Show all singer backgrounds when blending voices")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 3
                        Layout.leftMargin: mirrored ? 0 : pianoRollLayout.subOptionIndent
                        Layout.rightMargin: mirrored ? pianoRollLayout.subOptionIndent : 0
                        enabled: page.showSingerBackground
                        checked: page.showAllSingerBackgrounds
                        onClicked: page.showAllSingerBackgrounds = checked
                    }

                    Label {
                        id: singerBackgroundSpacingLabel
                        enabled: page.showSingerBackground && page.showAllSingerBackgrounds
                        Layout.leftMargin: mirrored ? 0 : 2 * pianoRollLayout.subOptionIndent
                        Layout.rightMargin: mirrored ? 2 * pianoRollLayout.subOptionIndent : 0
                        text: qsTr("Singer background spacing")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Slider {
                        enabled: singerBackgroundSpacingLabel.enabled
                        Layout.fillWidth: true
                        Layout.horizontalStretchFactor: 1
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
                        Layout.fillWidth: true
                        enabled: singerBackgroundSpacingLabel.enabled
                        from: 0
                        to: 100
                        value: page.singerBackgroundSpacing
                        Accessible.name: singerBackgroundSpacingLabel.text
                        onValueModified: page.singerBackgroundSpacing = value
                    }

                    CheckBox {
                        text: qsTr("Show note boundaries in the phoneme panel")
                        TextMatcherItem on text { matcher: page.matcher }
                        Layout.columnSpan: 3
                        checked: page.showPhonemeNoteBoundaryOverlay
                        onClicked: page.showPhonemeNoteBoundaryOverlay = checked
                    }

                    Label {
                        id: parameterPanelOverlayLabel
                        Layout.columnSpan: 2
                        text: qsTr("Parameter panel overlay")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        id: parameterPanelOverlayComboBox
                        Layout.fillWidth: true
                        implicitContentWidthPolicy: ComboBox.WidestText
                        model: [qsTr("First phoneme"), qsTr("Note boundaries"), qsTr("None")]
                        currentIndex: page.parameterPanelOverlayMode
                        Accessible.name: parameterPanelOverlayLabel.text
                        onActivated: (index) => page.parameterPanelOverlayMode = index
                    }

                    Label {
                        text: qsTr("Bottom panel opacity")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    Slider {
                        Layout.fillWidth: true
                        Layout.horizontalStretchFactor: 1
                        from: 0
                        to: 1
                        value: page.bottomPanelOpacity
                        onMoved: page.bottomPanelOpacity = value
                        ThemedItem.sliderTrackStartType: SVS.TS_Begin
                        ThemedItem.onDoubleClickReset: page.bottomPanelOpacity = 0.5
                    }
                    DoubleSpinBox {
                        Layout.fillWidth: true
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
                title: qsTr("Parameter Transforms")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                GridLayout {
                    anchors.fill: parent
                    columns: 2

                    Label {
                        id: preferredShiftLabel
                        text: qsTr("Preferred shift operation")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: [qsTr("Translate values"), qsTr("Scale absolute values")]
                        currentIndex: page.preferredParameterShiftMode
                        onActivated: index => page.preferredParameterShiftMode = index
                        Accessible.name: preferredShiftLabel.text
                    }
                    Label {
                        id: preferredScaleLabel
                        text: qsTr("Preferred scale operation")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: [qsTr("Endpoint line"), qsTr("Arithmetic mean")]
                        currentIndex: page.preferredParameterScaleMode
                        onActivated: index => page.preferredParameterScaleMode = index
                        Accessible.name: preferredScaleLabel.text
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
