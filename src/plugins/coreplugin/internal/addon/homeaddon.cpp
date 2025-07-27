#include "homeaddon.h"

#include <QQmlComponent>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/icore.h>
#include <coreplugin/ihomewindow.h>

namespace Core::Internal {
    HomeAddon::HomeAddon(QObject *parent) : IWindowAddOn(parent) {
    }
    HomeAddon::~HomeAddon() = default;
    void HomeAddon::initialize() {
        QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "Actions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.create();
        auto iWin = windowHandle()->cast<IHomeWindow>();
        iWin->actionContext()->addAction("core.newFile", o->property("newFile").value<QQmlComponent *>());
        iWin->actionContext()->addAction("core.openFile", o->property("openFile").value<QQmlComponent *>());
    }
    void HomeAddon::extensionsInitialized() {
    }
    bool HomeAddon::delayedInitialize() {
        return IWindowAddOn::delayedInitialize();
    }
}