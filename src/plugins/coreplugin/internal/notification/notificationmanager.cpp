#include "notificationmanager.h"

#include <QApplication>
#include <QTimer>

#include <CoreApi/plugindatabase.h>

#include <uishell/BubbleNotificationHandle.h>

#include <coreplugin/icore.h>
#include <coreplugin/notificationmessage.h>

namespace Core::Internal {

    static const char settingCategoryC[] = "Core::Internal::NotificationManager";

    NotificationManager::NotificationManager(IProjectWindow *parent) : QObject(parent) {
        parent->setProperty(settingCategoryC, QVariant::fromValue(this));
    }
    NotificationManager::~NotificationManager() = default;
    NotificationManager *NotificationManager::of(IProjectWindow *windowHandle) {
        return windowHandle->property(settingCategoryC).value<NotificationManager *>();
    }

    void NotificationManager::addMessage(NotificationMessage *message,
                                         IProjectWindow::NotificationBubbleMode mode) {
        auto settings = PluginDatabase::settings();
        settings->beginGroup(settingCategoryC);
        int autoHideTimeout = settings->value("autoHideTimeout", 5000).toInt();
        bool beepOnNotification = settings->value("beepOnNotification", false).toBool();
        settings->endGroup();

        auto handle = message->property("handle").value<UIShell::BubbleNotificationHandle *>();

        if (mode == IProjectWindow::AutoHide) {
            auto timer = new QTimer(handle);
            timer->setInterval(autoHideTimeout);
            connect(handle, &UIShell::BubbleNotificationHandle::hoverEntered, timer, &QTimer::stop);
            connect(handle, &UIShell::BubbleNotificationHandle::hoverExited, timer,
                    [=] { timer->start(); });
            connect(handle, &UIShell::BubbleNotificationHandle::hideClicked, timer,
                    &QObject::deleteLater);
            connect(handle, &UIShell::BubbleNotificationHandle::closeClicked, timer,
                    &QObject::deleteLater);
            timer->start();
        }

        const auto removeMessageFromBubbles = [=] {
            auto index = m_bubbleMessages.indexOf(message);
            if (index == -1)
                return;
            m_bubbleMessages.removeAt(index);
            emit messageRemovedFromBubbles(index, message);
        };

        const auto removeMessage = [=] {
            removeMessageFromBubbles();
            auto index = m_messages.indexOf(message);
            if (index == -1)
                return;
            m_messages.removeAt(index);
            emit messageRemoved(index, message);
        };

        connect(handle, &UIShell::BubbleNotificationHandle::hideClicked, this,
                removeMessageFromBubbles);
        connect(handle, &UIShell::BubbleNotificationHandle::closeClicked, this, removeMessage);
        connect(handle, &QObject::destroyed, this, removeMessage);

        if (mode != IProjectWindow::DoNotShowBubble) {
            m_bubbleMessages.append(message);
        }
        m_messages.append(message);

        if (beepOnNotification) {
            QApplication::beep();
        }

        emit messageAdded(m_messages.size() - 1, message);
        emit messageAddedToBubbles(m_messages.size() - 1, message);
    }
    QList<NotificationMessage *> NotificationManager::messages() const {
        return m_messages;
    }
    QList<NotificationMessage *> NotificationManager::bubbleMessages() const {
        return m_bubbleMessages;
    }
}