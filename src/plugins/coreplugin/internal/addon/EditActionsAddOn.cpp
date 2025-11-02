#include "EditActionsAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    EditActionsAddOn::EditActionsAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    EditActionsAddOn::~EditActionsAddOn() = default;

    void EditActionsAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditActionsAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void EditActionsAddOn::extensionsInitialized() {
    }

    bool EditActionsAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

}
