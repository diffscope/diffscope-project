#include "MixerAddOn.h"

#include <QQmlComponent>
#include <QVariant>
#include <QtGlobal>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/MixerPanelInterface.h>

namespace VisualEditor::Internal {

    MixerAddOn::MixerAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    MixerAddOn::~MixerAddOn() = default;

    void MixerAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        new MixerPanelInterface(this, windowInterface);

        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "MixerAddOnActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            });
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            o->setParent(this);
            QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
        }

        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "MixerPanel", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            });
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            o->setParent(this);
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.mixer", o->property("mixerPanelComponent").value<QQmlComponent *>());
        }
    }

    void MixerAddOn::extensionsInitialized() {
    }

    bool MixerAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    MixerPanelInterface *MixerAddOn::mixerPanelInterface() const {
        return MixerPanelInterface::of(windowHandle()->cast<Core::ProjectWindowInterface>());
    }

}

#include "moc_MixerAddOn.cpp"
