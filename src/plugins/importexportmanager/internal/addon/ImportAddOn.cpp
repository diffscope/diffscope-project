#include "ImportAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace ImportExportManager::Internal {
    ImportAddOn::ImportAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ImportAddOn::~ImportAddOn() = default;

    void ImportAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        
        // Load and register ImportAddOnActions QML component
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.ImportExportManager", "ImportAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({});
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void ImportAddOn::extensionsInitialized() {
    }

    bool ImportAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}

#include "moc_ImportAddOn.cpp"