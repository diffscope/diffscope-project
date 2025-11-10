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

}