#include "editactionsaddon.h"

#include <QQmlComponent>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/iprojectwindow.h>

namespace Core::Internal {
    EditActionsAddOn::EditActionsAddOn(QObject *parent) : IWindowAddOn(parent) {
    }

    EditActionsAddOn::~EditActionsAddOn() = default;

    void EditActionsAddOn::initialize() {
        auto iWin = windowHandle()->cast<IProjectWindow>();
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "EditActionsAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", iWin->actionContext());
    }

    void EditActionsAddOn::extensionsInitialized() {
    }

    bool EditActionsAddOn::delayedInitialize() {
        return IWindowAddOn::delayedInitialize();
    }
}