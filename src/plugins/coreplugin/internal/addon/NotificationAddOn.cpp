#include "NotificationAddOn.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftQuick/StatusTextContext.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/NotificationManager.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {
    NotificationAddOn::NotificationAddOn(QObject *parent) {
    }
    NotificationAddOn::~NotificationAddOn() = default;
    void NotificationAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        m_notificationManager = NotificationManager::of(windowInterface);
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "NotificationAddOnHelper");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto helper = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
            helper->setParent(windowInterface->window());
        }
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "NotificationsPanel", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                                                               {"addOn", QVariant::fromValue(this)},
                                                           },
                                                           RuntimeInterface::qmlEngine()->rootContext());
            o->setParent(this);
            windowInterface->actionContext()->addAction("core.panel.notifications", o->property("notificationsPanelComponent").value<QQmlComponent *>());
        }
        auto updateStatusText = [=] {
            auto statusContext = SVS::StatusTextContext::statusContext(qobject_cast<QQuickWindow *>(windowInterface->window()));
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
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        windowInterface->window()->setProperty("notificationEnablesAnimation", true);
        return WindowInterfaceAddOn::delayedInitialize();
    }
    NotificationManager *NotificationAddOn::notificationManager() const {
        return m_notificationManager;
    }
}

#include "moc_NotificationAddOn.cpp"
