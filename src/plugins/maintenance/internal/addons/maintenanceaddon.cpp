#include "maintenanceaddon.h"

#include <QDesktopServices>
#include <QQmlComponent>
#include <QQuickItem>

#include <application_config.h>

#include <SVSCraftGui/DesktopServices.h>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/logger.h>
#include <CoreApi/applicationinfo.h>

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

    static const QUrl ISSUE_URL("https://github.com/diffscope/diffscope-project/issues");
    static const QUrl CONTRIBUTE_URL("https://diffscope.org/contribute");
    static const QUrl COMMUNITY_URL("https://diffscope.org/community");
    static const QUrl RELEASE_LOG_URL("https://diffscope.org/releases/" APPLICATION_SEMVER);

    void MaintenanceAddOn::reveal(RevealFlag flag) {
        switch (flag) {
            case Logs:
                SVS::DesktopServices::reveal(Core::Logger::logsLocation());
                break;
            case Data:
                SVS::DesktopServices::reveal(Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::RuntimeData));
                break;
            case Plugins:
                SVS::DesktopServices::reveal(Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::BuiltinPlugins));
                break;
            case Issue:
                QDesktopServices::openUrl(ISSUE_URL);
                break;
            case Contribute:
                QDesktopServices::openUrl(CONTRIBUTE_URL);
                break;
            case Community:
                QDesktopServices::openUrl(COMMUNITY_URL);
                break;
            case ReleaseLog:
                QDesktopServices::openUrl(RELEASE_LOG_URL);
                break;
        }
    }

}
