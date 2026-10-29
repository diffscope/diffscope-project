// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_EDITORPREFERENCE_H
#define DIFFSCOPE_VISUALEDITOR_EDITORPREFERENCE_H

#include <qqmlintegration.h>

#include <QObject>

#include <ScopicFlowCore/ParameterEditorInteractionController.h>

class QQmlEngine;
class QJSEngine;

namespace VisualEditor::Internal {

    class VisualEditorPlugin;

    class EditorPreferencePrivate;

    class EditorPreference : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_DECLARE_PRIVATE(EditorPreference)
        Q_PROPERTY(EditorPreference::ScrollModifier alternateAxisModifier READ alternateAxisModifier WRITE setAlternateAxisModifier NOTIFY alternateAxisModifierChanged)
        Q_PROPERTY(EditorPreference::ScrollModifier zoomModifier READ zoomModifier WRITE setZoomModifier NOTIFY zoomModifierChanged)
        Q_PROPERTY(EditorPreference::ScrollModifier pageModifier READ pageModifier WRITE setPageModifier NOTIFY pageModifierChanged)
        Q_PROPERTY(bool usePageModifierAsAlternateAxisZoom READ usePageModifierAsAlternateAxisZoom WRITE setUsePageModifierAsAlternateAxisZoom NOTIFY usePageModifierAsAlternateAxisZoomChanged)
        Q_PROPERTY(bool middleButtonAutoScroll READ middleButtonAutoScroll WRITE setMiddleButtonAutoScroll NOTIFY middleButtonAutoScrollChanged)
        Q_PROPERTY(sflow::ParameterEditorInteractionController::ShiftMode preferredParameterShiftMode READ preferredParameterShiftMode WRITE setPreferredParameterShiftMode NOTIFY preferredParameterShiftModeChanged)
        Q_PROPERTY(sflow::ParameterEditorInteractionController::ScaleMode preferredParameterScaleMode READ preferredParameterScaleMode WRITE setPreferredParameterScaleMode NOTIFY preferredParameterScaleModeChanged)
        Q_PROPERTY(int autoDurationPositionAlignment READ autoDurationPositionAlignment WRITE setAutoDurationPositionAlignment NOTIFY autoDurationPositionAlignmentChanged)
        Q_PROPERTY(bool enableTemporarySnapOff READ enableTemporarySnapOff WRITE setEnableTemporarySnapOff NOTIFY enableTemporarySnapOffChanged)
        Q_PROPERTY(bool trackListOnRight READ trackListOnRight WRITE setTrackListOnRight NOTIFY trackListOnRightChanged)
        Q_PROPERTY(bool centerPianoRollOnClipDoubleClick READ centerPianoRollOnClipDoubleClick WRITE setCenterPianoRollOnClipDoubleClick NOTIFY centerPianoRollOnClipDoubleClickChanged)
        Q_PROPERTY(bool pianoRollAutoPageVerticalAdjustment READ pianoRollAutoPageVerticalAdjustment WRITE setPianoRollAutoPageVerticalAdjustment NOTIFY pianoRollAutoPageVerticalAdjustmentChanged)
        Q_PROPERTY(EditorPreference::PianoRollAutoPageOverflowBehavior pianoRollAutoPageOverflowBehavior READ pianoRollAutoPageOverflowBehavior WRITE setPianoRollAutoPageOverflowBehavior NOTIFY pianoRollAutoPageOverflowBehaviorChanged)
        Q_PROPERTY(double pianoKeyboardBlackKeyLengthRatio READ pianoKeyboardBlackKeyLengthRatio WRITE setPianoKeyboardBlackKeyLengthRatio NOTIFY pianoKeyboardBlackKeyLengthRatioChanged)
        Q_PROPERTY(EditorPreference::PianoKeyboardLabelPolicy pianoKeyboardLabelPolicy READ pianoKeyboardLabelPolicy WRITE setPianoKeyboardLabelPolicy NOTIFY pianoKeyboardLabelPolicyChanged)
        Q_PROPERTY(EditorPreference::NoteEditPitchCurveDisplayMode noteEditPitchCurveDisplayMode READ noteEditPitchCurveDisplayMode WRITE setNoteEditPitchCurveDisplayMode NOTIFY noteEditPitchCurveDisplayModeChanged)
        Q_PROPERTY(double autoPitchCurveDisplayPixelDensityThreshold READ autoPitchCurveDisplayPixelDensityThreshold WRITE setAutoPitchCurveDisplayPixelDensityThreshold NOTIFY autoPitchCurveDisplayPixelDensityThresholdChanged)
        Q_PROPERTY(bool displayPronunciationBelowNote READ displayPronunciationBelowNote WRITE setDisplayPronunciationBelowNote NOTIFY displayPronunciationBelowNoteChanged)
        Q_PROPERTY(int shortNoteThreshold READ shortNoteThreshold WRITE setShortNoteThreshold NOTIFY shortNoteThresholdChanged)
        Q_PROPERTY(bool warnOfOverlappingNotes READ warnOfOverlappingNotes WRITE setWarnOfOverlappingNotes NOTIFY warnOfOverlappingNotesChanged)
        Q_PROPERTY(bool trackCursorPosition READ trackCursorPosition WRITE setTrackCursorPosition NOTIFY trackCursorPositionChanged)
        Q_PROPERTY(bool showSingerBackground READ showSingerBackground WRITE setShowSingerBackground NOTIFY showSingerBackgroundChanged)
        Q_PROPERTY(double singerBackgroundOpacity READ singerBackgroundOpacity WRITE setSingerBackgroundOpacity NOTIFY singerBackgroundOpacityChanged)
        Q_PROPERTY(EditorPreference::SingerBackgroundAlignment singerBackgroundAlignment READ singerBackgroundAlignment WRITE setSingerBackgroundAlignment NOTIFY singerBackgroundAlignmentChanged)
        Q_PROPERTY(int singerBackgroundHorizontalMargin READ singerBackgroundHorizontalMargin WRITE setSingerBackgroundHorizontalMargin NOTIFY singerBackgroundHorizontalMarginChanged)
        Q_PROPERTY(int singerBackgroundHorizontalOffset READ singerBackgroundHorizontalOffset WRITE setSingerBackgroundHorizontalOffset NOTIFY singerBackgroundHorizontalOffsetChanged)
        Q_PROPERTY(int singerBackgroundVerticalMargin READ singerBackgroundVerticalMargin WRITE setSingerBackgroundVerticalMargin NOTIFY singerBackgroundVerticalMarginChanged)
        Q_PROPERTY(bool showAllSingerBackgrounds READ showAllSingerBackgrounds WRITE setShowAllSingerBackgrounds NOTIFY showAllSingerBackgroundsChanged)
        Q_PROPERTY(int singerBackgroundSpacing READ singerBackgroundSpacing WRITE setSingerBackgroundSpacing NOTIFY singerBackgroundSpacingChanged)
        Q_PROPERTY(double bottomPanelOpacity READ bottomPanelOpacity WRITE setBottomPanelOpacity NOTIFY bottomPanelOpacityChanged)

    public:
        using ParameterShiftMode = sflow::ParameterEditorInteractionController::ShiftMode;
        using ParameterScaleMode = sflow::ParameterEditorInteractionController::ScaleMode;

        ~EditorPreference() override;

        static EditorPreference *instance();

        static inline EditorPreference *create(QQmlEngine *, QJSEngine *) {
            return instance();
        }

        void load();
        Q_INVOKABLE void save() const;

        enum PianoKeyboardLabelPolicy {
            LP_All,
            LP_COnly,
            LP_None,
        };
        Q_ENUM(PianoKeyboardLabelPolicy)

        enum NoteEditPitchCurveDisplayMode {
            PCDM_Automatic,
            PCDM_AlwaysShow,
            PCDM_AlwaysHide,
        };
        Q_ENUM(NoteEditPitchCurveDisplayMode)

        enum ScrollModifier {
            SM_Control,
            SM_Alt,
            SM_Shift,
        };
        Q_ENUM(ScrollModifier)

        enum PianoRollAutoPageOverflowBehavior {
            APOB_AlignTop,
            APOB_AlignBottom,
            APOB_Center,
            APOB_ZoomToFit,
        };
        Q_ENUM(PianoRollAutoPageOverflowBehavior)

        enum SingerBackgroundAlignment {
            SBA_Left,
            SBA_Center,
            SBA_Right,
        };
        Q_ENUM(SingerBackgroundAlignment)

        static ScrollModifier alternateAxisModifier();
        static void setAlternateAxisModifier(ScrollModifier alternateAxisModifier);

        static ScrollModifier zoomModifier();
        static void setZoomModifier(ScrollModifier zoomModifier);

        static ScrollModifier pageModifier();
        static void setPageModifier(ScrollModifier pageModifier);

        static bool usePageModifierAsAlternateAxisZoom();
        static void setUsePageModifierAsAlternateAxisZoom(bool usePageModifierAsAlternateAxisZoom);

        static bool middleButtonAutoScroll();
        static void setMiddleButtonAutoScroll(bool middleButtonAutoScroll);

        static ParameterShiftMode preferredParameterShiftMode();
        static void setPreferredParameterShiftMode(ParameterShiftMode mode);

        static ParameterScaleMode preferredParameterScaleMode();
        static void setPreferredParameterScaleMode(ParameterScaleMode mode);

        static int autoDurationPositionAlignment();
        static void setAutoDurationPositionAlignment(int autoDurationPositionAlignment);

        static bool enableTemporarySnapOff();
        static void setEnableTemporarySnapOff(bool enableTemporarySnapOff);

        static bool trackListOnRight();
        static void setTrackListOnRight(bool trackListOnRight);

        static bool centerPianoRollOnClipDoubleClick();
        static void setCenterPianoRollOnClipDoubleClick(bool centerPianoRollOnClipDoubleClick);

        static bool pianoRollAutoPageVerticalAdjustment();
        static void setPianoRollAutoPageVerticalAdjustment(bool enabled);

        static PianoRollAutoPageOverflowBehavior pianoRollAutoPageOverflowBehavior();
        static void setPianoRollAutoPageOverflowBehavior(PianoRollAutoPageOverflowBehavior behavior);

        static double pianoKeyboardBlackKeyLengthRatio();
        static void setPianoKeyboardBlackKeyLengthRatio(double pianoKeyboardBlackKeyLengthRatio);

        static PianoKeyboardLabelPolicy pianoKeyboardLabelPolicy();
        static void setPianoKeyboardLabelPolicy(PianoKeyboardLabelPolicy pianoKeyboardLabelPolicy);

        static NoteEditPitchCurveDisplayMode noteEditPitchCurveDisplayMode();
        static void setNoteEditPitchCurveDisplayMode(NoteEditPitchCurveDisplayMode noteEditPitchCurveDisplayMode);

        static double autoPitchCurveDisplayPixelDensityThreshold();
        static void setAutoPitchCurveDisplayPixelDensityThreshold(double autoPitchCurveDisplayPixelDensityThreshold);

        static bool displayPronunciationBelowNote();
        static void setDisplayPronunciationBelowNote(bool displayPronunciationBelowNote);

        static int shortNoteThreshold();
        static void setShortNoteThreshold(int shortNoteThreshold);

        static bool warnOfOverlappingNotes();
        static void setWarnOfOverlappingNotes(bool warnOfOverlappingNotes);

        static bool trackCursorPosition();
        static void setTrackCursorPosition(bool trackCursorPosition);

        static bool showSingerBackground();
        static void setShowSingerBackground(bool showSingerBackground);

        static double singerBackgroundOpacity();
        static void setSingerBackgroundOpacity(double singerBackgroundOpacity);

        static SingerBackgroundAlignment singerBackgroundAlignment();
        static void setSingerBackgroundAlignment(SingerBackgroundAlignment singerBackgroundAlignment);

        static int singerBackgroundHorizontalMargin();
        static void setSingerBackgroundHorizontalMargin(int singerBackgroundHorizontalMargin);

        static int singerBackgroundHorizontalOffset();
        static void setSingerBackgroundHorizontalOffset(int singerBackgroundHorizontalOffset);

        static int singerBackgroundVerticalMargin();
        static void setSingerBackgroundVerticalMargin(int singerBackgroundVerticalMargin);

        static bool showAllSingerBackgrounds();
        static void setShowAllSingerBackgrounds(bool showAllSingerBackgrounds);

        static int singerBackgroundSpacing();
        static void setSingerBackgroundSpacing(int singerBackgroundSpacing);

        static double bottomPanelOpacity();
        static void setBottomPanelOpacity(double bottomPanelOpacity);

    Q_SIGNALS:
        void alternateAxisModifierChanged();
        void zoomModifierChanged();
        void pageModifierChanged();
        void usePageModifierAsAlternateAxisZoomChanged();
        void middleButtonAutoScrollChanged();
        void preferredParameterShiftModeChanged();
        void preferredParameterScaleModeChanged();
        void autoDurationPositionAlignmentChanged();
        void enableTemporarySnapOffChanged();
        void trackListOnRightChanged();
        void centerPianoRollOnClipDoubleClickChanged();
        void pianoRollAutoPageVerticalAdjustmentChanged();
        void pianoRollAutoPageOverflowBehaviorChanged();
        void pianoKeyboardBlackKeyLengthRatioChanged();
        void pianoKeyboardLabelPolicyChanged();
        void noteEditPitchCurveDisplayModeChanged();
        void autoPitchCurveDisplayPixelDensityThresholdChanged();
        void displayPronunciationBelowNoteChanged();
        void shortNoteThresholdChanged();
        void warnOfOverlappingNotesChanged();
        void trackCursorPositionChanged();
        void showSingerBackgroundChanged();
        void singerBackgroundOpacityChanged();
        void singerBackgroundAlignmentChanged();
        void singerBackgroundHorizontalMarginChanged();
        void singerBackgroundHorizontalOffsetChanged();
        void singerBackgroundVerticalMarginChanged();
        void showAllSingerBackgroundsChanged();
        void singerBackgroundSpacingChanged();
        void bottomPanelOpacityChanged();

    private:
        friend class VisualEditorPlugin;
        explicit EditorPreference(QObject *parent = nullptr);
        QScopedPointer<EditorPreferencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_EDITORPREFERENCE_H
