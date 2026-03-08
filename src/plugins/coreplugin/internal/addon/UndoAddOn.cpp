#include "UndoAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    UndoAddOn::UndoAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    UndoAddOn::~UndoAddOn() = default;

    void UndoAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "UndoAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void UndoAddOn::extensionsInitialized() {
    }

    bool UndoAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

}
