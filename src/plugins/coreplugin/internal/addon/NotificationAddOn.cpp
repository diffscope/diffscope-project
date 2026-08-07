#include "NotificationAddOn.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftQuick/StatusTextContext.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/NotificationCenter.h>
#include <coreplugin/internal/NotificationManager.h>
#include <coreplugin/internal/NotificationViewModel.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {
    NotificationAddOn::NotificationAddOn(QObject *parent) {
    }
    NotificationAddOn::~NotificationAddOn() = default;
    void NotificationAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        auto localNotificationManager = NotificationManager::of(windowInterface);
        m_notificationViewModel = new NotificationViewModel({
            NotificationCenter::instance()->globalNotificationManager(),
            localNotificationManager,
        }, this);
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "NotificationAddOnHelper");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto helper = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
            helper->setParent(windowInterface->window());
        }
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "NotificationAddOnActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            });
            o->setParent(this);
            QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
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
            windowInterface->actionContext()->addAction("org.diffscope.core.panel.notifications", o->property("notificationsPanelComponent").value<QQmlComponent *>());
        }
        auto updateStatusText = [=] {
            if (windowInterface->isEffectivelyClosed()) {
                return;
            }
            auto statusContext = SVS::StatusTextContext::statusContext(qobject_cast<QQuickWindow *>(windowInterface->window()));
            if (m_notificationViewModel->messages().isEmpty()) {
                statusContext->pop(this);
            } else if (m_notificationViewModel->messages().size() == 1) {
                statusContext->update(this, m_notificationViewModel->topMessageTitle());
            } else {
                statusContext->update(this, tr("%1 (+%Ln notification(s))", nullptr, m_notificationViewModel->messages().size() - 1).arg(m_notificationViewModel->topMessageTitle()));
            }
        };
        connect(m_notificationViewModel, &NotificationViewModel::messageAdded, this, updateStatusText);
        connect(m_notificationViewModel, &NotificationViewModel::messageRemoved, this, updateStatusText);
        updateStatusText();
    }
    void NotificationAddOn::extensionsInitialized() {
    }
    bool NotificationAddOn::delayedInitialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        windowInterface->window()->setProperty("notificationEnablesAnimation", true);
        return WindowInterfaceAddOn::delayedInitialize();
    }
    NotificationViewModel *NotificationAddOn::notificationManager() const {
        return m_notificationViewModel;
    }
}

#include "moc_NotificationAddOn.cpp"
