#include "ArrangementAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ArrangementPanelInterface.h>

namespace VisualEditor::Internal {
    ArrangementAddOn::ArrangementAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ArrangementAddOn::~ArrangementAddOn() = default;

    void ArrangementAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        new ArrangementPanelInterface(windowInterface);
        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "ArrangementPanel", this);
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
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.arrangement", o->property("arrangementPanelComponent").value<QQmlComponent *>());
        }

        // TODO
        windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.pianoRoll", new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "PianoRollPanel", this));
        windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.mixer", new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "MixerPanel", this));
    }

    void ArrangementAddOn::extensionsInitialized() {}

    bool ArrangementAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
    ArrangementPanelInterface *ArrangementAddOn::arrangementPanelInterface() const {
        return ArrangementPanelInterface::of(windowHandle()->cast<Core::ProjectWindowInterface>());
    }
}

#include "moc_ArrangementAddOn.cpp"
