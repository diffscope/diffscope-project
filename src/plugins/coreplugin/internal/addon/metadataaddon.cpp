#include "metadataaddon.h"

#include <QQmlComponent>
#include <QQmlEngine>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/coreinterface.h>
#include <coreplugin/projectwindowinterface.h>

namespace Core::Internal {
    MetadataAddOn::MetadataAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    MetadataAddOn::~MetadataAddOn() = default;

    void MetadataAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        
        // Create MetadataPanel component and add it to action context
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "MetadataPanel", this);
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        }, RuntimeInterface::qmlEngine()->rootContext());
        o->setParent(this);
        windowInterface->actionContext()->addAction("core.panel.metadata", o->property("metadataPanelComponent").value<QQmlComponent *>());
    }

    void MetadataAddOn::extensionsInitialized() {
    }

    bool MetadataAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}

#include "moc_metadataaddon.cpp"