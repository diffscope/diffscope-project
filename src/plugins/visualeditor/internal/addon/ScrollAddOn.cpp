#include "ScrollAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ArrangementPanelInterface.h>
#include <visualeditor/PianoRollPanelInterface.h>
#include <visualeditor/MixerPanelInterface.h>

namespace VisualEditor::Internal {
    ScrollAddOn::ScrollAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ScrollAddOn::~ScrollAddOn() = default;

    void ScrollAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);
        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "ScrollAddOnActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            });
            o->setParent(this);
            QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
            m_actionsObject = o;
        }
    }

    void ScrollAddOn::extensionsInitialized() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        m_actionsObject->setProperty("arrangementPanelInterface", QVariant::fromValue(ArrangementPanelInterface::of(windowInterface)));
        m_actionsObject->setProperty("pianoRollPanelInterface", QVariant::fromValue(PianoRollPanelInterface::of(windowInterface)));
        m_actionsObject->setProperty("mixerPanelInterface", QVariant::fromValue(MixerPanelInterface::of(windowInterface)));
    }

    bool ScrollAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    ScrollAddOn::ActiveEditingArea ScrollAddOn::activeEditingArea() const {
        return m_activeEditingArea;
    }

    void ScrollAddOn::setActiveEditingArea(ActiveEditingArea area) {
        if (m_activeEditingArea != area) {
            m_activeEditingArea = area;
            Q_EMIT activeEditingAreaChanged();
        }
    }
}
