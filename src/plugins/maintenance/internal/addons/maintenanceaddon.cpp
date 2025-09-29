#include "maintenanceaddon.h"

#include <QQmlComponent>
#include <QQuickItem>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/actionwindowinterfacebase.h>

#include <maintenance/internal/maintenanceplugin.h>

namespace Maintenance {
    MaintenanceAddOn::MaintenanceAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    MaintenanceAddOn::~MaintenanceAddOn() = default;

    static QPointer<QQuickWindow> m_window;
    static MaintenancePlugin *m_plugin = nullptr;

    void MaintenanceAddOn::setPlugin(MaintenancePlugin *plugin) {
        m_plugin = plugin;
    }

    QQuickWindow *MaintenanceAddOn::window() {
        if (!m_window) {
            auto pageComponents = Core::RuntimeInterface::instance()->getObjects("org.diffscope.maintenance.pages");
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
                QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Maintenance", "MaintenanceDialog");
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

    void MaintenanceAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Maintenance", "MaintenanceActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)}
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void MaintenanceAddOn::extensionsInitialized() {
    }

    bool MaintenanceAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
    void MaintenanceAddOn::execMaintenance() {
        QEventLoop eventLoop;
        auto win = window();
        connect(win, SIGNAL(finished()), &eventLoop, SLOT(quit()));
        win->show();
        eventLoop.exec();
        win->deleteLater();
    }
}
