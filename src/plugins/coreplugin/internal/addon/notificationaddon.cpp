#include "notificationaddon.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>

#include <SVSCraftQuick/StatusTextContext.h>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

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
            QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "NotificationAddOnHelper");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto helper = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)}
            });
            helper->setParent(iWin->window());
        }
        {
            QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "NotificationsPanel", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            }, PluginDatabase::qmlEngine()->rootContext());
            o->setParent(this);
            iWin->actionContext()->addAction("core.panel.notifications", o->property("notificationsPanelComponent").value<QQmlComponent *>());
        }
        auto updateStatusText = [=] {
            auto statusContext = SVS::StatusTextContext::statusContext(qobject_cast<QQuickWindow *>(iWin->window()));
            if (m_notificationManager->messages().isEmpty()) {
                statusContext->pop(this);
            } else if (m_notificationManager->messages().size() == 1) {
                statusContext->update(this, m_notificationManager->topMessageTitle());
            } else {
                statusContext->update(this, tr("%1 (+%Ln notification(s))", nullptr, m_notificationManager->messages().size() - 1).arg(m_notificationManager->topMessageTitle()));
            }
        };
        connect(m_notificationManager, &NotificationManager::messageAdded, this, updateStatusText);
        connect(m_notificationManager, &NotificationManager::messageRemoved, this, updateStatusText);
    }
    void NotificationAddOn::extensionsInitialized() {
    }
    bool NotificationAddOn::delayedInitialize() {
        auto iWin = windowHandle()->cast<IProjectWindow>();
        iWin->window()->setProperty("notificationEnablesAnimation", true);
        return IWindowAddOn::delayedInitialize();
    }
    NotificationManager *NotificationAddOn::notificationManager() const {
        return m_notificationManager;
    }
}

#include "moc_notificationaddon.cpp"