#include "welcomewizardaddon.h"

#include <QQmlComponent>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/iactionwindowbase.h>

namespace WelcomeWizard {
    WelcomeWizardAddOn::WelcomeWizardAddOn(QObject *parent) : Core::IWindowAddOn(parent) {
    }

    WelcomeWizardAddOn::~WelcomeWizardAddOn() = default;

    static QQuickWindow *m_window = nullptr;

    QQuickWindow *WelcomeWizardAddOn::window() {
        return m_window;
    }
    void WelcomeWizardAddOn::setWindow(QQuickWindow *w) {
        m_window = w;
    }

    void WelcomeWizardAddOn::initialize() {
        auto iWin = windowHandle()->cast<Core::IActionWindowBase>();
        QQmlComponent component(Core::PluginDatabase::qmlEngine(), "DiffScope.WelcomeWizard", "WelcomeWizardActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)}
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", iWin->actionContext());
    }

    void WelcomeWizardAddOn::extensionsInitialized() {
    }

    bool WelcomeWizardAddOn::delayedInitialize() {
        return IWindowAddOn::delayedInitialize();
    }
}