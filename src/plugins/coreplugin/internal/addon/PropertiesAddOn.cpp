#include "PropertiesAddOn.h"

#include <QQmlComponent>
#include <QQmlEngine>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {
    PropertiesAddOn::PropertiesAddOn(QObject *parent) {
    }
    PropertiesAddOn::~PropertiesAddOn() = default;
    void PropertiesAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "PropertiesPanel", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            },
            RuntimeInterface::qmlEngine()->rootContext());
            o->setParent(this);
            windowInterface->actionContext()->addAction("org.diffscope.core.panel.properties", o->property("propertiesPanelComponent").value<QQmlComponent *>());
        }
    }
    void PropertiesAddOn::extensionsInitialized() {
    }
    bool PropertiesAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
    QList<QQmlComponent *> PropertiesAddOn::getComponents(const QString &id) {
        auto list = RuntimeInterface::instance()->getObjects("org.diffscope.core.propertyeditor." + id);
        QList<QQmlComponent *> result;
        for (auto obj : list) {
            if (auto component = qobject_cast<QQmlComponent *>(obj)) {
                result.append(component);
            }
        }
        return result;
    }
}

#include "moc_PropertiesAddOn.cpp"
