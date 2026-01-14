#include "InsertItemAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    InsertItemAddOn::InsertItemAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    InsertItemAddOn::~InsertItemAddOn() = default;

    void InsertItemAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "InsertItemAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void InsertItemAddOn::extensionsInitialized() {
    }

    bool InsertItemAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

}
