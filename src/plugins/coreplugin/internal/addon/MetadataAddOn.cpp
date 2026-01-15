#include "MetadataAddOn.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QMetaObject>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {
    MetadataAddOn::MetadataAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    MetadataAddOn::~MetadataAddOn() = default;

    void MetadataAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();

        // Create MetadataPanel component and add it to action context
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "MetadataPanel", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            },
            RuntimeInterface::qmlEngine()->rootContext());
            o->setParent(this);
            windowInterface->actionContext()->addAction("org.diffscope.core.panel.metadata", o->property("metadataPanelComponent").value<QQmlComponent *>());
        }

        // Register metadata actions defined in QML
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "MetadataAddOnActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            });
            o->setParent(this);
            QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
        }
    }

    void MetadataAddOn::extensionsInitialized() {
    }

    bool MetadataAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}

#include "moc_MetadataAddOn.cpp"
