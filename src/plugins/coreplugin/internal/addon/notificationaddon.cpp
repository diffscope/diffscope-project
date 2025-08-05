#include "notificationaddon.h"

#include <QQmlComponent>
#include <QQmlEngine>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/icore.h>
#include <coreplugin/internal/notificationmanager.h>
#include <coreplugin/iprojectwindow.h>

namespace Core::Internal {
    NotificationAddOn::NotificationAddOn(QObject *parent) {
    }
    NotificationAddOn::~NotificationAddOn() = default;
    void NotificationAddOn::initialize() {
        auto iWin = windowHandle()->cast<IProjectWindow>();
        m_notificationManager = NotificationManager::of(iWin);
        {
            QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "NotificationAddOnHelper");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto helper = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)}
            });
            helper->setParent(iWin->window());
        }
        {
            QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "NotificationsPanel", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            }, ICore::qmlEngine()->rootContext());
            o->setParent(this);
            iWin->actionContext()->addAction("core.notificationsPanel", o->property("notificationsPanelComponent").value<QQmlComponent *>());
        }
    }
    void NotificationAddOn::extensionsInitialized() {
    }
    bool NotificationAddOn::delayedInitialize() {
        return IWindowAddOn::delayedInitialize();
    }
    NotificationManager *NotificationAddOn::notificationManager() const {
        return m_notificationManager;
    }
}

#include "moc_notificationaddon.cpp"