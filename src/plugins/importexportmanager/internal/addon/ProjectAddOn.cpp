#include "ProjectAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace ImportExportManager::Internal {
    ProjectAddOn::ProjectAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ProjectAddOn::~ProjectAddOn() = default;

    void ProjectAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        
        // Load and register ProjectAddOnActions QML component
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.ImportExportManager", "ProjectAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void ProjectAddOn::extensionsInitialized() {
    }

    bool ProjectAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}

#include "moc_ProjectAddOn.cpp"