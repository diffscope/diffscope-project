// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EditorPreference.h"

#include <QSettings>

#include <QtGlobal>

#include <CoreApi/runtimeinterface.h>

namespace VisualEditor::Internal {

    static constexpr double kDefaultAutoPitchCurveDisplayPixelDensityThreshold = 0.07071;

    class EditorPreferencePrivate {
    public:
        bool initialized{};

        EditorPreference::ScrollModifier alternateAxisModifier{};
        EditorPreference::ScrollModifier zoomModifier{};
        EditorPreference::ScrollModifier pageModifier{};
        bool usePageModifierAsAlternateAxisZoom{};
        bool middleButtonAutoScroll{};
        int autoDurationPositionAlignment{24};
        bool enableTemporarySnapOff{true};
        bool trackListOnRight{};
        bool centerPianoRollOnClipDoubleClick{true};
        bool pianoRollAutoPageVerticalAdjustment{false};
        EditorPreference::PianoRollAutoPageOverflowBehavior pianoRollAutoPageOverflowBehavior{EditorPreference::APOB_AlignTop};
        double pianoKeyboardBlackKeyLengthRatio{0.6};
        EditorPreference::PianoKeyboardLabelPolicy pianoKeyboardLabelPolicy{EditorPreference::LP_COnly};
        EditorPreference::NoteEditPitchCurveDisplayMode noteEditPitchCurveDisplayMode{};
        double autoPitchCurveDisplayPixelDensityThreshold{kDefaultAutoPitchCurveDisplayPixelDensityThreshold};
        bool displayPronunciationBelowNote{};
        int shortNoteThreshold{30};
        bool warnOfOverlappingNotes{true};
        bool trackCursorPosition{true};
        bool showSingerBackground{true};
        double singerBackgroundOpacity{0.25};
        EditorPreference::SingerBackgroundAlignment singerBackgroundAlignment{EditorPreference::SBA_Right};
        int singerBackgroundHorizontalMargin{32};
        int singerBackgroundHorizontalOffset{};
        int singerBackgroundVerticalMargin{32};
        bool showAllSingerBackgrounds{true};
        int singerBackgroundSpacing{16};
        double bottomPanelOpacity{0.5};
    };

    static EditorPreference *m_instance = nullptr;

    EditorPreference::EditorPreference(QObject *parent) : QObject(parent), d_ptr(new EditorPreferencePrivate) {
        Q_ASSERT(!m_instance);
        m_instance = this;
    }

    EditorPreference::~EditorPreference() {
        m_instance = nullptr;
    }

    EditorPreference *EditorPreference::instance() {
        return m_instance;
    }

    void EditorPreference::load() {
        Q_D(EditorPreference);
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        d->alternateAxisModifier = settings->value("alternateAxisModifier", QVariant::fromValue(SM_Alt)).value<ScrollModifier>();
        emit alternateAxisModifierChanged();
        d->zoomModifier = settings->value("zoomModifier", QVariant::fromValue(SM_Control)).value<ScrollModifier>();
        emit zoomModifierChanged();
        d->pageModifier = settings->value("pageModifier", QVariant::fromValue(SM_Shift)).value<ScrollModifier>();
        emit pageModifierChanged();
        d->usePageModifierAsAlternateAxisZoom = settings->value("usePageModifierAsAlternateAxisZoom", false).toBool();
        emit usePageModifierAsAlternateAxisZoomChanged();
        d->middleButtonAutoScroll = settings->value("middleButtonAutoScroll", false).toBool();
        emit middleButtonAutoScrollChanged();
        d->autoDurationPositionAlignment = settings->value("autoDurationPositionAlignment", 24).toInt();
        emit autoDurationPositionAlignmentChanged();
        d->enableTemporarySnapOff = settings->value("enableTemporarySnapOff", true).toBool();
        emit enableTemporarySnapOffChanged();
        d->trackListOnRight = settings->value("trackListOnRight", false).toBool();
        emit trackListOnRightChanged();
        d->centerPianoRollOnClipDoubleClick = settings->value("centerPianoRollOnClipDoubleClick", true).toBool();
        emit centerPianoRollOnClipDoubleClickChanged();
        d->pianoRollAutoPageVerticalAdjustment = settings->value("pianoRollAutoPageVerticalAdjustment", false).toBool();
        Q_EMIT pianoRollAutoPageVerticalAdjustmentChanged();
        d->pianoRollAutoPageOverflowBehavior = settings->value("pianoRollAutoPageOverflowBehavior", QVariant::fromValue(APOB_AlignTop)).value<PianoRollAutoPageOverflowBehavior>();
        Q_EMIT pianoRollAutoPageOverflowBehaviorChanged();
        d->pianoKeyboardBlackKeyLengthRatio = settings->value("pianoKeyboardBlackKeyLengthRatio", 0.6).toDouble();
        emit pianoKeyboardBlackKeyLengthRatioChanged();
        d->pianoKeyboardLabelPolicy = settings->value("pianoKeyboardLabelPolicy", QVariant::fromValue(LP_COnly)).value<PianoKeyboardLabelPolicy>();
        emit pianoKeyboardLabelPolicyChanged();
        d->noteEditPitchCurveDisplayMode = settings->value("noteEditPitchCurveDisplayMode", QVariant::fromValue(PCDM_Automatic)).value<NoteEditPitchCurveDisplayMode>();
        Q_EMIT noteEditPitchCurveDisplayModeChanged();
        d->autoPitchCurveDisplayPixelDensityThreshold = settings->value("autoPitchCurveDisplayPixelDensityThreshold", kDefaultAutoPitchCurveDisplayPixelDensityThreshold).toDouble();
        Q_EMIT autoPitchCurveDisplayPixelDensityThresholdChanged();
        d->displayPronunciationBelowNote = settings->value("displayPronunciationBelowNote", false).toBool();
        emit displayPronunciationBelowNoteChanged();
        d->shortNoteThreshold = settings->value("shortNoteThreshold", 30).toInt();
        emit shortNoteThresholdChanged();
        d->warnOfOverlappingNotes = settings->value("warnOfOverlappingNotes", true).toBool();
        emit warnOfOverlappingNotesChanged();
        d->trackCursorPosition = settings->value("trackCursorPosition", true).toBool();
        emit trackCursorPositionChanged();
        d->showSingerBackground = settings->value("showSingerBackground", true).toBool();
        emit showSingerBackgroundChanged();
        d->singerBackgroundOpacity = qBound(
            0.0, settings->value("singerBackgroundOpacity", 0.25).toDouble(), 1.0);
        emit singerBackgroundOpacityChanged();
        d->singerBackgroundAlignment = settings->value("singerBackgroundAlignment", QVariant::fromValue(SBA_Right)).value<SingerBackgroundAlignment>();
        Q_EMIT singerBackgroundAlignmentChanged();
        d->singerBackgroundHorizontalMargin = settings->value("singerBackgroundHorizontalMargin", 32).toInt();
        Q_EMIT singerBackgroundHorizontalMarginChanged();
        d->singerBackgroundHorizontalOffset = settings->value("singerBackgroundHorizontalOffset", 0).toInt();
        Q_EMIT singerBackgroundHorizontalOffsetChanged();
        d->singerBackgroundVerticalMargin = settings->value("singerBackgroundVerticalMargin", 32).toInt();
        Q_EMIT singerBackgroundVerticalMarginChanged();
        d->showAllSingerBackgrounds = settings->value("showAllSingerBackgrounds", true).toBool();
        Q_EMIT showAllSingerBackgroundsChanged();
        d->singerBackgroundSpacing = settings->value("singerBackgroundSpacing", 16).toInt();
        Q_EMIT singerBackgroundSpacingChanged();
        d->bottomPanelOpacity = qBound(
            0.0, settings->value("bottomPanelOpacity", 0.5).toDouble(), 1.0);
        emit bottomPanelOpacityChanged();
        settings->endGroup();
    }

    void EditorPreference::save() const {
        Q_D(const EditorPreference);
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue("alternateAxisModifier", static_cast<int>(d->alternateAxisModifier));
        settings->setValue("zoomModifier", static_cast<int>(d->zoomModifier));
        settings->setValue("pageModifier", static_cast<int>(d->pageModifier));
        settings->setValue("usePageModifierAsAlternateAxisZoom", d->usePageModifierAsAlternateAxisZoom);
        settings->setValue("middleButtonAutoScroll", d->middleButtonAutoScroll);
        settings->setValue("autoDurationPositionAlignment", d->autoDurationPositionAlignment);
        settings->setValue("enableTemporarySnapOff", d->enableTemporarySnapOff);
        settings->setValue("trackListOnRight", d->trackListOnRight);
        settings->setValue("centerPianoRollOnClipDoubleClick", d->centerPianoRollOnClipDoubleClick);
        settings->setValue("pianoRollAutoPageVerticalAdjustment", d->pianoRollAutoPageVerticalAdjustment);
        settings->setValue("pianoRollAutoPageOverflowBehavior", static_cast<int>(d->pianoRollAutoPageOverflowBehavior));
        settings->setValue("pianoKeyboardBlackKeyLengthRatio", d->pianoKeyboardBlackKeyLengthRatio);
        settings->setValue("pianoKeyboardLabelPolicy", static_cast<int>(d->pianoKeyboardLabelPolicy));
        settings->setValue("noteEditPitchCurveDisplayMode", static_cast<int>(d->noteEditPitchCurveDisplayMode));
        settings->setValue("autoPitchCurveDisplayPixelDensityThreshold", d->autoPitchCurveDisplayPixelDensityThreshold);
        settings->setValue("displayPronunciationBelowNote", d->displayPronunciationBelowNote);
        settings->setValue("shortNoteThreshold", d->shortNoteThreshold);
        settings->setValue("warnOfOverlappingNotes", d->warnOfOverlappingNotes);
        settings->setValue("trackCursorPosition", d->trackCursorPosition);
        settings->setValue("showSingerBackground", d->showSingerBackground);
        settings->setValue("singerBackgroundOpacity", d->singerBackgroundOpacity);
        settings->setValue("singerBackgroundAlignment", static_cast<int>(d->singerBackgroundAlignment));
        settings->setValue("singerBackgroundHorizontalMargin", d->singerBackgroundHorizontalMargin);
        settings->setValue("singerBackgroundHorizontalOffset", d->singerBackgroundHorizontalOffset);
        settings->setValue("singerBackgroundVerticalMargin", d->singerBackgroundVerticalMargin);
        settings->setValue("showAllSingerBackgrounds", d->showAllSingerBackgrounds);
        settings->setValue("singerBackgroundSpacing", d->singerBackgroundSpacing);
        settings->setValue("bottomPanelOpacity", d->bottomPanelOpacity);
        settings->endGroup();
    }

#define M_INSTANCE_D      \
    Q_ASSERT(m_instance); \
    auto d = m_instance->d_func()

    EditorPreference::ScrollModifier EditorPreference::alternateAxisModifier() {
        M_INSTANCE_D;
        return d->alternateAxisModifier;
    }

    void EditorPreference::setAlternateAxisModifier(ScrollModifier alternateAxisModifier) {
        M_INSTANCE_D;
        if (d->alternateAxisModifier == alternateAxisModifier)
            return;
        d->alternateAxisModifier = alternateAxisModifier;
        emit m_instance->alternateAxisModifierChanged();
    }

    EditorPreference::ScrollModifier EditorPreference::zoomModifier() {
        M_INSTANCE_D;
        return d->zoomModifier;
    }

    void EditorPreference::setZoomModifier(ScrollModifier zoomModifier) {
        M_INSTANCE_D;
        if (d->zoomModifier == zoomModifier)
            return;
        d->zoomModifier = zoomModifier;
        emit m_instance->zoomModifierChanged();
    }

    EditorPreference::ScrollModifier EditorPreference::pageModifier() {
        M_INSTANCE_D;
        return d->pageModifier;
    }

    void EditorPreference::setPageModifier(ScrollModifier pageModifier) {
        M_INSTANCE_D;
        if (d->pageModifier == pageModifier)
            return;
        d->pageModifier = pageModifier;
        emit m_instance->pageModifierChanged();
    }

    bool EditorPreference::usePageModifierAsAlternateAxisZoom() {
        M_INSTANCE_D;
        return d->usePageModifierAsAlternateAxisZoom;
    }

    void EditorPreference::setUsePageModifierAsAlternateAxisZoom(bool usePageModifierAsAlternateAxisZoom) {
        M_INSTANCE_D;
        if (d->usePageModifierAsAlternateAxisZoom == usePageModifierAsAlternateAxisZoom)
            return;
        d->usePageModifierAsAlternateAxisZoom = usePageModifierAsAlternateAxisZoom;
        emit m_instance->usePageModifierAsAlternateAxisZoomChanged();
    }

    bool EditorPreference::middleButtonAutoScroll() {
        M_INSTANCE_D;
        return d->middleButtonAutoScroll;
    }

    void EditorPreference::setMiddleButtonAutoScroll(bool middleButtonAutoScroll) {
        M_INSTANCE_D;
        if (d->middleButtonAutoScroll == middleButtonAutoScroll)
            return;
        d->middleButtonAutoScroll = middleButtonAutoScroll;
        emit m_instance->middleButtonAutoScrollChanged();
    }

    int EditorPreference::autoDurationPositionAlignment() {
        M_INSTANCE_D;
        return d->autoDurationPositionAlignment;
    }

    void EditorPreference::setAutoDurationPositionAlignment(int autoDurationPositionAlignment) {
        M_INSTANCE_D;
        if (d->autoDurationPositionAlignment == autoDurationPositionAlignment)
            return;
        d->autoDurationPositionAlignment = autoDurationPositionAlignment;
        emit m_instance->autoDurationPositionAlignmentChanged();
    }

    bool EditorPreference::enableTemporarySnapOff() {
        M_INSTANCE_D;
        return d->enableTemporarySnapOff;
    }

    void EditorPreference::setEnableTemporarySnapOff(bool enableTemporarySnapOff) {
        M_INSTANCE_D;
        if (d->enableTemporarySnapOff == enableTemporarySnapOff)
            return;
        d->enableTemporarySnapOff = enableTemporarySnapOff;
        emit m_instance->enableTemporarySnapOffChanged();
    }

    bool EditorPreference::trackListOnRight() {
        M_INSTANCE_D;
        return d->trackListOnRight;
    }

    void EditorPreference::setTrackListOnRight(bool trackListOnRight) {
        M_INSTANCE_D;
        if (d->trackListOnRight == trackListOnRight)
            return;
        d->trackListOnRight = trackListOnRight;
        emit m_instance->trackListOnRightChanged();
    }

    bool EditorPreference::centerPianoRollOnClipDoubleClick() {
        M_INSTANCE_D;
        return d->centerPianoRollOnClipDoubleClick;
    }

    void EditorPreference::setCenterPianoRollOnClipDoubleClick(bool centerPianoRollOnClipDoubleClick) {
        M_INSTANCE_D;
        if (d->centerPianoRollOnClipDoubleClick == centerPianoRollOnClipDoubleClick)
            return;
        d->centerPianoRollOnClipDoubleClick = centerPianoRollOnClipDoubleClick;
        emit m_instance->centerPianoRollOnClipDoubleClickChanged();
    }

    bool EditorPreference::pianoRollAutoPageVerticalAdjustment() {
        M_INSTANCE_D;
        return d->pianoRollAutoPageVerticalAdjustment;
    }

    void EditorPreference::setPianoRollAutoPageVerticalAdjustment(bool enabled) {
        M_INSTANCE_D;
        if (d->pianoRollAutoPageVerticalAdjustment == enabled)
            return;
        d->pianoRollAutoPageVerticalAdjustment = enabled;
        Q_EMIT m_instance->pianoRollAutoPageVerticalAdjustmentChanged();
    }

    EditorPreference::PianoRollAutoPageOverflowBehavior EditorPreference::pianoRollAutoPageOverflowBehavior() {
        M_INSTANCE_D;
        return d->pianoRollAutoPageOverflowBehavior;
    }

    void EditorPreference::setPianoRollAutoPageOverflowBehavior(PianoRollAutoPageOverflowBehavior behavior) {
        M_INSTANCE_D;
        if (d->pianoRollAutoPageOverflowBehavior == behavior)
            return;
        d->pianoRollAutoPageOverflowBehavior = behavior;
        Q_EMIT m_instance->pianoRollAutoPageOverflowBehaviorChanged();
    }

    double EditorPreference::pianoKeyboardBlackKeyLengthRatio() {
        M_INSTANCE_D;
        return d->pianoKeyboardBlackKeyLengthRatio;
    }

    void EditorPreference::setPianoKeyboardBlackKeyLengthRatio(double pianoKeyboardBlackKeyLengthRatio) {
        M_INSTANCE_D;
        if (qFuzzyCompare(d->pianoKeyboardBlackKeyLengthRatio, pianoKeyboardBlackKeyLengthRatio))
            return;
        d->pianoKeyboardBlackKeyLengthRatio = pianoKeyboardBlackKeyLengthRatio;
        emit m_instance->pianoKeyboardBlackKeyLengthRatioChanged();
    }

    EditorPreference::PianoKeyboardLabelPolicy EditorPreference::pianoKeyboardLabelPolicy() {
        M_INSTANCE_D;
        return d->pianoKeyboardLabelPolicy;
    }

    void EditorPreference::setPianoKeyboardLabelPolicy(PianoKeyboardLabelPolicy pianoKeyboardLabelPolicy) {
        M_INSTANCE_D;
        if (d->pianoKeyboardLabelPolicy == pianoKeyboardLabelPolicy)
            return;
        d->pianoKeyboardLabelPolicy = pianoKeyboardLabelPolicy;
        emit m_instance->pianoKeyboardLabelPolicyChanged();
    }

    EditorPreference::NoteEditPitchCurveDisplayMode EditorPreference::noteEditPitchCurveDisplayMode() {
        M_INSTANCE_D;
        return d->noteEditPitchCurveDisplayMode;
    }

    void EditorPreference::setNoteEditPitchCurveDisplayMode(NoteEditPitchCurveDisplayMode noteEditPitchCurveDisplayMode) {
        M_INSTANCE_D;
        if (d->noteEditPitchCurveDisplayMode == noteEditPitchCurveDisplayMode)
            return;
        d->noteEditPitchCurveDisplayMode = noteEditPitchCurveDisplayMode;
        Q_EMIT m_instance->noteEditPitchCurveDisplayModeChanged();
    }

    double EditorPreference::autoPitchCurveDisplayPixelDensityThreshold() {
        M_INSTANCE_D;
        return d->autoPitchCurveDisplayPixelDensityThreshold;
    }

    void EditorPreference::setAutoPitchCurveDisplayPixelDensityThreshold(double autoPitchCurveDisplayPixelDensityThreshold) {
        M_INSTANCE_D;
        if (qFuzzyCompare(d->autoPitchCurveDisplayPixelDensityThreshold, autoPitchCurveDisplayPixelDensityThreshold))
            return;
        d->autoPitchCurveDisplayPixelDensityThreshold = autoPitchCurveDisplayPixelDensityThreshold;
        Q_EMIT m_instance->autoPitchCurveDisplayPixelDensityThresholdChanged();
    }

    bool EditorPreference::displayPronunciationBelowNote() {
        M_INSTANCE_D;
        return d->displayPronunciationBelowNote;
    }

    void EditorPreference::setDisplayPronunciationBelowNote(bool displayPronunciationBelowNote) {
        M_INSTANCE_D;
        if (d->displayPronunciationBelowNote == displayPronunciationBelowNote)
            return;
        d->displayPronunciationBelowNote = displayPronunciationBelowNote;
        emit m_instance->displayPronunciationBelowNoteChanged();
    }

    int EditorPreference::shortNoteThreshold() {
        M_INSTANCE_D;
        return d->shortNoteThreshold;
    }

    void EditorPreference::setShortNoteThreshold(int shortNoteThreshold) {
        M_INSTANCE_D;
        if (d->shortNoteThreshold == shortNoteThreshold)
            return;
        d->shortNoteThreshold = shortNoteThreshold;
        emit m_instance->shortNoteThresholdChanged();
    }

    bool EditorPreference::warnOfOverlappingNotes() {
        M_INSTANCE_D;
        return d->warnOfOverlappingNotes;
    }

    void EditorPreference::setWarnOfOverlappingNotes(bool warnOfOverlappingNotes) {
        M_INSTANCE_D;
        if (d->warnOfOverlappingNotes == warnOfOverlappingNotes)
            return;
        d->warnOfOverlappingNotes = warnOfOverlappingNotes;
        emit m_instance->warnOfOverlappingNotesChanged();
    }

    bool EditorPreference::trackCursorPosition() {
        M_INSTANCE_D;
        return d->trackCursorPosition;
    }
    void EditorPreference::setTrackCursorPosition(bool trackCursorPosition) {
        M_INSTANCE_D;
        if (d->trackCursorPosition == trackCursorPosition)
            return;
        d->trackCursorPosition = trackCursorPosition;
        emit m_instance->trackCursorPositionChanged();
    }

    bool EditorPreference::showSingerBackground() {
        M_INSTANCE_D;
        return d->showSingerBackground;
    }

    void EditorPreference::setShowSingerBackground(bool showSingerBackground) {
        M_INSTANCE_D;
        if (d->showSingerBackground == showSingerBackground)
            return;
        d->showSingerBackground = showSingerBackground;
        emit m_instance->showSingerBackgroundChanged();
    }

    double EditorPreference::singerBackgroundOpacity() {
        M_INSTANCE_D;
        return d->singerBackgroundOpacity;
    }

    void EditorPreference::setSingerBackgroundOpacity(double singerBackgroundOpacity) {
        M_INSTANCE_D;
        singerBackgroundOpacity = qBound(0.0, singerBackgroundOpacity, 1.0);
        if (qFuzzyCompare(d->singerBackgroundOpacity, singerBackgroundOpacity))
            return;
        d->singerBackgroundOpacity = singerBackgroundOpacity;
        emit m_instance->singerBackgroundOpacityChanged();
    }

    EditorPreference::SingerBackgroundAlignment EditorPreference::singerBackgroundAlignment() {
        M_INSTANCE_D;
        return d->singerBackgroundAlignment;
    }

    void EditorPreference::setSingerBackgroundAlignment(SingerBackgroundAlignment singerBackgroundAlignment) {
        M_INSTANCE_D;
        if (d->singerBackgroundAlignment == singerBackgroundAlignment)
            return;
        d->singerBackgroundAlignment = singerBackgroundAlignment;
        Q_EMIT m_instance->singerBackgroundAlignmentChanged();
    }

    int EditorPreference::singerBackgroundHorizontalMargin() {
        M_INSTANCE_D;
        return d->singerBackgroundHorizontalMargin;
    }

    void EditorPreference::setSingerBackgroundHorizontalMargin(int singerBackgroundHorizontalMargin) {
        M_INSTANCE_D;
        if (d->singerBackgroundHorizontalMargin == singerBackgroundHorizontalMargin)
            return;
        d->singerBackgroundHorizontalMargin = singerBackgroundHorizontalMargin;
        Q_EMIT m_instance->singerBackgroundHorizontalMarginChanged();
    }

    int EditorPreference::singerBackgroundHorizontalOffset() {
        M_INSTANCE_D;
        return d->singerBackgroundHorizontalOffset;
    }

    void EditorPreference::setSingerBackgroundHorizontalOffset(int singerBackgroundHorizontalOffset) {
        M_INSTANCE_D;
        if (d->singerBackgroundHorizontalOffset == singerBackgroundHorizontalOffset)
            return;
        d->singerBackgroundHorizontalOffset = singerBackgroundHorizontalOffset;
        Q_EMIT m_instance->singerBackgroundHorizontalOffsetChanged();
    }

    int EditorPreference::singerBackgroundVerticalMargin() {
        M_INSTANCE_D;
        return d->singerBackgroundVerticalMargin;
    }

    void EditorPreference::setSingerBackgroundVerticalMargin(int singerBackgroundVerticalMargin) {
        M_INSTANCE_D;
        if (d->singerBackgroundVerticalMargin == singerBackgroundVerticalMargin)
            return;
        d->singerBackgroundVerticalMargin = singerBackgroundVerticalMargin;
        Q_EMIT m_instance->singerBackgroundVerticalMarginChanged();
    }

    bool EditorPreference::showAllSingerBackgrounds() {
        M_INSTANCE_D;
        return d->showAllSingerBackgrounds;
    }

    void EditorPreference::setShowAllSingerBackgrounds(bool showAllSingerBackgrounds) {
        M_INSTANCE_D;
        if (d->showAllSingerBackgrounds == showAllSingerBackgrounds)
            return;
        d->showAllSingerBackgrounds = showAllSingerBackgrounds;
        Q_EMIT m_instance->showAllSingerBackgroundsChanged();
    }

    int EditorPreference::singerBackgroundSpacing() {
        M_INSTANCE_D;
        return d->singerBackgroundSpacing;
    }

    void EditorPreference::setSingerBackgroundSpacing(int singerBackgroundSpacing) {
        M_INSTANCE_D;
        if (d->singerBackgroundSpacing == singerBackgroundSpacing)
            return;
        d->singerBackgroundSpacing = singerBackgroundSpacing;
        Q_EMIT m_instance->singerBackgroundSpacingChanged();
    }

    double EditorPreference::bottomPanelOpacity() {
        M_INSTANCE_D;
        return d->bottomPanelOpacity;
    }

    void EditorPreference::setBottomPanelOpacity(double bottomPanelOpacity) {
        M_INSTANCE_D;
        bottomPanelOpacity = qBound(0.0, bottomPanelOpacity, 1.0);
        if (qFuzzyCompare(d->bottomPanelOpacity, bottomPanelOpacity))
            return;
        d->bottomPanelOpacity = bottomPanelOpacity;
        emit m_instance->bottomPanelOpacityChanged();
    }

}
