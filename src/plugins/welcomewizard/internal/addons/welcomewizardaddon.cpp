#include "welcomewizardaddon.h"

#include <QQmlComponent>
#include <QQuickItem>

#include <CoreApi/runtimeInterface.h>

#include <coreplugin/actionwindowinterfacebase.h>

#include <welcomewizard/internal/welcomewizardplugin.h>

namespace WelcomeWizard {
    WelcomeWizardAddOn::WelcomeWizardAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    WelcomeWizardAddOn::~WelcomeWizardAddOn() = default;

    static QPointer<QQuickWindow> m_window;
    static WelcomeWizardPlugin *m_plugin = nullptr;

    void WelcomeWizardAddOn::setPlugin(WelcomeWizardPlugin *plugin) {
        m_plugin = plugin;
    }

    QQuickWindow *WelcomeWizardAddOn::window() {
        if (!m_window) {
            auto pageComponents = Core::RuntimeInterface::instance()->getObjects("org.diffscope.welcomewizard.pages");
            QList<QQuickItem *> pages;
            for (auto o : pageComponents) {
                auto c = qobject_cast<QQmlComponent *>(o);
                if (!c || c->isError()) {
                    continue;
                }
                std::unique_ptr<QObject> obj(c->create(c->creationContext()));
                if (auto item = qobject_cast<QQuickItem *>(obj.get())) {
                    Q_UNUSED(obj.release());
                    pages.append(item);
                }
            }
            {
                QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.WelcomeWizard", "WelcomeWizardDialog");
                if (component.isError()) {
                    qFatal() << component.errorString();
                }
                auto o = component.createWithInitialProperties({
                    {"pages", QVariant::fromValue(pages)}
                });
                o->setParent(m_plugin);
                m_window = qobject_cast<QQuickWindow *>(o);
            }
            for (auto item : pages) {
                item->setParent(m_window);
            }
        }
        return m_window;
    }

    void WelcomeWizardAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.WelcomeWizard", "WelcomeWizardActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)}
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void WelcomeWizardAddOn::extensionsInitialized() {
    }

    bool WelcomeWizardAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
    void WelcomeWizardAddOn::execWelcomeWizard() {
        QEventLoop eventLoop;
        auto win = window();
        connect(win, SIGNAL(finished()), &eventLoop, SLOT(quit()));
        win->show();
        eventLoop.exec();
        win->deleteLater();
    }
}
