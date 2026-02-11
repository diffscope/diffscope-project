#include "EditorPreference.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>

namespace VisualEditor::Internal {

    class EditorPreferencePrivate {
    public:
        bool initialized{};

        EditorPreference::ScrollModifier alternateAxisModifier{};
        EditorPreference::ScrollModifier zoomModifier{};
        EditorPreference::ScrollModifier pageModifier{};
        bool usePageModifierAsAlternateAxisZoom{};
        bool middleButtonAutoScroll{};
        int autoDurationPositionAlignment{48};
        bool enableTemporarySnapOff{true};
        bool trackListOnRight{};
        bool pianoKeyboardUseSimpleStyle{};
        EditorPreference::PianoKeyboardLabelPolicy pianoKeyboardLabelPolicy{};
        bool trackCursorPosition{true};
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
        d->autoDurationPositionAlignment = settings->value("autoDurationPositionAlignment", 48).toInt();
        emit autoDurationPositionAlignmentChanged();
        d->enableTemporarySnapOff = settings->value("enableTemporarySnapOff", true).toBool();
        emit enableTemporarySnapOffChanged();
        d->trackListOnRight = settings->value("trackListOnRight", false).toBool();
        emit trackListOnRightChanged();
        d->pianoKeyboardUseSimpleStyle = settings->value("pianoKeyboardUseSimpleStyle", false).toBool();
        emit pianoKeyboardUseSimpleStyleChanged();
        d->pianoKeyboardLabelPolicy = settings->value("pianoKeyboardLabelPolicy", QVariant::fromValue(LP_All)).value<PianoKeyboardLabelPolicy>();
        emit pianoKeyboardLabelPolicyChanged();
        d->trackCursorPosition = settings->value("trackCursorPosition", true).toBool();
        emit trackCursorPositionChanged();
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
        settings->setValue("pianoKeyboardUseSimpleStyle", d->pianoKeyboardUseSimpleStyle);
        settings->setValue("pianoKeyboardLabelPolicy", static_cast<int>(d->pianoKeyboardLabelPolicy));
        settings->setValue("trackCursorPosition", d->trackCursorPosition);
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

    bool EditorPreference::pianoKeyboardUseSimpleStyle() {
        M_INSTANCE_D;
        return d->pianoKeyboardUseSimpleStyle;
    }

    void EditorPreference::setPianoKeyboardUseSimpleStyle(bool pianoKeyboardUseSimpleStyle) {
        M_INSTANCE_D;
        if (d->pianoKeyboardUseSimpleStyle == pianoKeyboardUseSimpleStyle)
            return;
        d->pianoKeyboardUseSimpleStyle = pianoKeyboardUseSimpleStyle;
        emit m_instance->pianoKeyboardUseSimpleStyleChanged();
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

}
