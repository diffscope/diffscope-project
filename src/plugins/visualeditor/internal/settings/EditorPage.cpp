// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EditorPage.h"

#include <QLoggingCategory>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <visualeditor/internal/EditorPreference.h>

namespace VisualEditor::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcEditorPage, "diffscope.visualeditor.editorpage")

    EditorPage::EditorPage(QObject *parent) : Core::ISettingPage("org.diffscope.visualeditor.Editor", parent) {
        setTitle(tr("Editor"));
        setDescription(tr("Configure editor appearance and interaction behaviors"));
    }

    EditorPage::~EditorPage() {
        delete m_widget;
    }

    QString EditorPage::sortKeyword() const {
        return QStringLiteral("Editor");
    }

    bool EditorPage::matches(const QString &word) {
        return Core::ISettingPage::matches(word) || widgetMatches(word);
    }

    QObject *EditorPage::widget() {
        if (m_widget)
            return m_widget;
        qCDebug(lcEditorPage) << "Creating widget";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "EditorPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        if (!m_widget) {
            qFatal() << component.errorString();
        }
        m_widget->setParent(this);
        return m_widget;
    }

    void EditorPage::beginSetting() {
        qCInfo(lcEditorPage) << "Beginning setting";
        widget();
        m_widget->setProperty("alternateAxisModifier", EditorPreference::instance()->property("alternateAxisModifier"));
        qCDebug(lcEditorPage) << m_widget->property("alternateAxisModifier");
        m_widget->setProperty("zoomModifier", EditorPreference::instance()->property("zoomModifier"));
        qCDebug(lcEditorPage) << m_widget->property("zoomModifier");
        m_widget->setProperty("pageModifier", EditorPreference::instance()->property("pageModifier"));
        qCDebug(lcEditorPage) << m_widget->property("pageModifier");
        m_widget->setProperty("usePageModifierAsAlternateAxisZoom", EditorPreference::instance()->property("usePageModifierAsAlternateAxisZoom"));
        qCDebug(lcEditorPage) << m_widget->property("usePageModifierAsAlternateAxisZoom");
        m_widget->setProperty("middleButtonAutoScroll", EditorPreference::instance()->property("middleButtonAutoScroll"));
        qCDebug(lcEditorPage) << m_widget->property("middleButtonAutoScroll");
        m_widget->setProperty("autoDurationPositionAlignment", EditorPreference::instance()->property("autoDurationPositionAlignment"));
        qCDebug(lcEditorPage) << m_widget->property("autoDurationPositionAlignment");
        m_widget->setProperty("enableTemporarySnapOff", EditorPreference::instance()->property("enableTemporarySnapOff"));
        qCDebug(lcEditorPage) << m_widget->property("enableTemporarySnapOff");
        m_widget->setProperty("trackListOnRight", EditorPreference::instance()->property("trackListOnRight"));
        qCDebug(lcEditorPage) << m_widget->property("trackListOnRight");
        m_widget->setProperty("centerPianoRollOnClipDoubleClick", EditorPreference::instance()->property("centerPianoRollOnClipDoubleClick"));
        qCDebug(lcEditorPage) << m_widget->property("centerPianoRollOnClipDoubleClick");
        m_widget->setProperty("pianoRollAutoPageVerticalAdjustment", EditorPreference::instance()->property("pianoRollAutoPageVerticalAdjustment"));
        qCDebug(lcEditorPage) << m_widget->property("pianoRollAutoPageVerticalAdjustment");
        m_widget->setProperty("pianoRollAutoPageOverflowBehavior", EditorPreference::instance()->property("pianoRollAutoPageOverflowBehavior"));
        qCDebug(lcEditorPage) << m_widget->property("pianoRollAutoPageOverflowBehavior");
        m_widget->setProperty("pianoKeyboardBlackKeyLengthRatio", EditorPreference::instance()->property("pianoKeyboardBlackKeyLengthRatio"));
        qCDebug(lcEditorPage) << m_widget->property("pianoKeyboardBlackKeyLengthRatio");
        m_widget->setProperty("pianoKeyboardLabelPolicy", EditorPreference::instance()->property("pianoKeyboardLabelPolicy"));
        qCDebug(lcEditorPage) << m_widget->property("pianoKeyboardLabelPolicy");
        m_widget->setProperty("noteEditPitchCurveDisplayMode", EditorPreference::instance()->property("noteEditPitchCurveDisplayMode"));
        qCDebug(lcEditorPage) << m_widget->property("noteEditPitchCurveDisplayMode");
        m_widget->setProperty("autoPitchCurveDisplayPixelDensityThreshold", EditorPreference::instance()->property("autoPitchCurveDisplayPixelDensityThreshold"));
        qCDebug(lcEditorPage) << m_widget->property("autoPitchCurveDisplayPixelDensityThreshold");
        m_widget->setProperty("displayPronunciationBelowNote", EditorPreference::instance()->property("displayPronunciationBelowNote"));
        qCDebug(lcEditorPage) << m_widget->property("displayPronunciationBelowNote");
        m_widget->setProperty("shortNoteThreshold", EditorPreference::instance()->property("shortNoteThreshold"));
        qCDebug(lcEditorPage) << m_widget->property("shortNoteThreshold");
        m_widget->setProperty("warnOfOverlappingNotes", EditorPreference::instance()->property("warnOfOverlappingNotes"));
        qCDebug(lcEditorPage) << m_widget->property("warnOfOverlappingNotes");
        m_widget->setProperty("trackCursorPosition", EditorPreference::instance()->property("trackCursorPosition"));
        qCDebug(lcEditorPage) << m_widget->property("trackCursorPosition");
        m_widget->setProperty("showSingerBackground", EditorPreference::instance()->property("showSingerBackground"));
        qCDebug(lcEditorPage) << m_widget->property("showSingerBackground");
        m_widget->setProperty("singerBackgroundOpacity", EditorPreference::instance()->property("singerBackgroundOpacity"));
        qCDebug(lcEditorPage) << m_widget->property("singerBackgroundOpacity");
        m_widget->setProperty("singerBackgroundAlignment", EditorPreference::instance()->property("singerBackgroundAlignment"));
        qCDebug(lcEditorPage) << m_widget->property("singerBackgroundAlignment");
        m_widget->setProperty("singerBackgroundHorizontalMargin", EditorPreference::instance()->property("singerBackgroundHorizontalMargin"));
        qCDebug(lcEditorPage) << m_widget->property("singerBackgroundHorizontalMargin");
        m_widget->setProperty("singerBackgroundHorizontalOffset", EditorPreference::instance()->property("singerBackgroundHorizontalOffset"));
        qCDebug(lcEditorPage) << m_widget->property("singerBackgroundHorizontalOffset");
        m_widget->setProperty("singerBackgroundVerticalMargin", EditorPreference::instance()->property("singerBackgroundVerticalMargin"));
        qCDebug(lcEditorPage) << m_widget->property("singerBackgroundVerticalMargin");
        m_widget->setProperty("showAllSingerBackgrounds", EditorPreference::instance()->property("showAllSingerBackgrounds"));
        qCDebug(lcEditorPage) << m_widget->property("showAllSingerBackgrounds");
        m_widget->setProperty("singerBackgroundSpacing", EditorPreference::instance()->property("singerBackgroundSpacing"));
        qCDebug(lcEditorPage) << m_widget->property("singerBackgroundSpacing");
        m_widget->setProperty("bottomPanelOpacity", EditorPreference::instance()->property("bottomPanelOpacity"));
        qCDebug(lcEditorPage) << m_widget->property("bottomPanelOpacity");
        m_widget->setProperty("started", true);
        Core::ISettingPage::beginSetting();
    }

    bool EditorPage::accept() {
        qCInfo(lcEditorPage) << "Accepting";
        qCDebug(lcEditorPage) << "alternateAxisModifier" << m_widget->property("alternateAxisModifier");
        EditorPreference::instance()->setProperty("alternateAxisModifier", m_widget->property("alternateAxisModifier"));
        qCDebug(lcEditorPage) << "zoomModifier" << m_widget->property("zoomModifier");
        EditorPreference::instance()->setProperty("zoomModifier", m_widget->property("zoomModifier"));
        qCDebug(lcEditorPage) << "pageModifier" << m_widget->property("pageModifier");
        EditorPreference::instance()->setProperty("pageModifier", m_widget->property("pageModifier"));
        qCDebug(lcEditorPage) << "usePageModifierAsAlternateAxisZoom" << m_widget->property("usePageModifierAsAlternateAxisZoom");
        EditorPreference::instance()->setProperty("usePageModifierAsAlternateAxisZoom", m_widget->property("usePageModifierAsAlternateAxisZoom"));
        qCDebug(lcEditorPage) << "middleButtonAutoScroll" << m_widget->property("middleButtonAutoScroll");
        EditorPreference::instance()->setProperty("middleButtonAutoScroll", m_widget->property("middleButtonAutoScroll"));
        qCDebug(lcEditorPage) << "autoDurationPositionAlignment" << m_widget->property("autoDurationPositionAlignment");
        EditorPreference::instance()->setProperty("autoDurationPositionAlignment", m_widget->property("autoDurationPositionAlignment"));
        qCDebug(lcEditorPage) << "enableTemporarySnapOff" << m_widget->property("enableTemporarySnapOff");
        EditorPreference::instance()->setProperty("enableTemporarySnapOff", m_widget->property("enableTemporarySnapOff"));
        qCDebug(lcEditorPage) << "trackListOnRight" << m_widget->property("trackListOnRight");
        EditorPreference::instance()->setProperty("trackListOnRight", m_widget->property("trackListOnRight"));
        qCDebug(lcEditorPage) << "centerPianoRollOnClipDoubleClick" << m_widget->property("centerPianoRollOnClipDoubleClick");
        EditorPreference::instance()->setProperty("centerPianoRollOnClipDoubleClick", m_widget->property("centerPianoRollOnClipDoubleClick"));
        qCDebug(lcEditorPage) << "pianoRollAutoPageVerticalAdjustment" << m_widget->property("pianoRollAutoPageVerticalAdjustment");
        EditorPreference::instance()->setProperty("pianoRollAutoPageVerticalAdjustment", m_widget->property("pianoRollAutoPageVerticalAdjustment"));
        qCDebug(lcEditorPage) << "pianoRollAutoPageOverflowBehavior" << m_widget->property("pianoRollAutoPageOverflowBehavior");
        EditorPreference::instance()->setProperty("pianoRollAutoPageOverflowBehavior", m_widget->property("pianoRollAutoPageOverflowBehavior"));
        qCDebug(lcEditorPage) << "pianoKeyboardBlackKeyLengthRatio" << m_widget->property("pianoKeyboardBlackKeyLengthRatio");
        EditorPreference::instance()->setProperty("pianoKeyboardBlackKeyLengthRatio", m_widget->property("pianoKeyboardBlackKeyLengthRatio"));
        qCDebug(lcEditorPage) << "pianoKeyboardLabelPolicy" << m_widget->property("pianoKeyboardLabelPolicy");
        EditorPreference::instance()->setProperty("pianoKeyboardLabelPolicy", m_widget->property("pianoKeyboardLabelPolicy"));
        qCDebug(lcEditorPage) << "noteEditPitchCurveDisplayMode" << m_widget->property("noteEditPitchCurveDisplayMode");
        EditorPreference::instance()->setProperty("noteEditPitchCurveDisplayMode", m_widget->property("noteEditPitchCurveDisplayMode"));
        qCDebug(lcEditorPage) << "autoPitchCurveDisplayPixelDensityThreshold" << m_widget->property("autoPitchCurveDisplayPixelDensityThreshold");
        EditorPreference::instance()->setProperty("autoPitchCurveDisplayPixelDensityThreshold", m_widget->property("autoPitchCurveDisplayPixelDensityThreshold"));
        qCDebug(lcEditorPage) << "displayPronunciationBelowNote" << m_widget->property("displayPronunciationBelowNote");
        EditorPreference::instance()->setProperty("displayPronunciationBelowNote", m_widget->property("displayPronunciationBelowNote"));
        qCDebug(lcEditorPage) << "shortNoteThreshold" << m_widget->property("shortNoteThreshold");
        EditorPreference::instance()->setProperty("shortNoteThreshold", m_widget->property("shortNoteThreshold"));
        qCDebug(lcEditorPage) << "warnOfOverlappingNotes" << m_widget->property("warnOfOverlappingNotes");
        EditorPreference::instance()->setProperty("warnOfOverlappingNotes", m_widget->property("warnOfOverlappingNotes"));
        qCDebug(lcEditorPage) << "trackCursorPosition" << m_widget->property("trackCursorPosition");
        EditorPreference::instance()->setProperty("trackCursorPosition", m_widget->property("trackCursorPosition"));
        qCDebug(lcEditorPage) << "showSingerBackground" << m_widget->property("showSingerBackground");
        EditorPreference::instance()->setProperty("showSingerBackground", m_widget->property("showSingerBackground"));
        qCDebug(lcEditorPage) << "singerBackgroundOpacity" << m_widget->property("singerBackgroundOpacity");
        EditorPreference::instance()->setProperty("singerBackgroundOpacity", m_widget->property("singerBackgroundOpacity"));
        qCDebug(lcEditorPage) << "singerBackgroundAlignment" << m_widget->property("singerBackgroundAlignment");
        EditorPreference::instance()->setProperty("singerBackgroundAlignment", m_widget->property("singerBackgroundAlignment"));
        qCDebug(lcEditorPage) << "singerBackgroundHorizontalMargin" << m_widget->property("singerBackgroundHorizontalMargin");
        EditorPreference::instance()->setProperty("singerBackgroundHorizontalMargin", m_widget->property("singerBackgroundHorizontalMargin"));
        qCDebug(lcEditorPage) << "singerBackgroundHorizontalOffset" << m_widget->property("singerBackgroundHorizontalOffset");
        EditorPreference::instance()->setProperty("singerBackgroundHorizontalOffset", m_widget->property("singerBackgroundHorizontalOffset"));
        qCDebug(lcEditorPage) << "singerBackgroundVerticalMargin" << m_widget->property("singerBackgroundVerticalMargin");
        EditorPreference::instance()->setProperty("singerBackgroundVerticalMargin", m_widget->property("singerBackgroundVerticalMargin"));
        qCDebug(lcEditorPage) << "showAllSingerBackgrounds" << m_widget->property("showAllSingerBackgrounds");
        EditorPreference::instance()->setProperty("showAllSingerBackgrounds", m_widget->property("showAllSingerBackgrounds"));
        qCDebug(lcEditorPage) << "singerBackgroundSpacing" << m_widget->property("singerBackgroundSpacing");
        EditorPreference::instance()->setProperty("singerBackgroundSpacing", m_widget->property("singerBackgroundSpacing"));
        qCDebug(lcEditorPage) << "bottomPanelOpacity" << m_widget->property("bottomPanelOpacity");
        EditorPreference::instance()->setProperty("bottomPanelOpacity", m_widget->property("bottomPanelOpacity"));
        EditorPreference::instance()->save();
        return Core::ISettingPage::accept();
    }

    void EditorPage::endSetting() {
        qCInfo(lcEditorPage) << "Ending setting";
        m_widget->setProperty("started", false);
        Core::ISettingPage::endSetting();
    }

    bool EditorPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}
