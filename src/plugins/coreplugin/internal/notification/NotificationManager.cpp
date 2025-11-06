#include "NotificationManager.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QTimer>

#include <CoreApi/runtimeinterface.h>

#include <uishell/BubbleNotificationHandle.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/NotificationMessage.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcNotificationManager, "diffscope.core.notificationmanager")

    NotificationManager::NotificationManager(ProjectWindowInterface *parent) : QObject(parent) {
        parent->setProperty(staticMetaObject.className(), QVariant::fromValue(this));
    }
    NotificationManager::~NotificationManager() = default;
    NotificationManager *NotificationManager::of(ProjectWindowInterface *windowHandle) {
        return windowHandle->property(staticMetaObject.className()).value<NotificationManager *>();
    }

    void NotificationManager::addMessage(NotificationMessage *message, ProjectWindowInterface::NotificationBubbleMode mode) {
        qCInfo(lcNotificationManager) << "Adding message:" << message << message->title() << message->text() << mode;

        int autoHideTimeout = BehaviorPreference::notificationAutoHideTimeout();
        bool beepOnNotification = BehaviorPreference::hasNotificationSoundAlert();

        auto handle = message->property("handle").value<UIShell::BubbleNotificationHandle *>();

        if (mode == ProjectWindowInterface::AutoHide) {
            auto timer = new QTimer(handle);
            timer->setInterval(autoHideTimeout);
            timer->setSingleShot(true);
            connect(handle, &UIShell::BubbleNotificationHandle::hoverEntered, timer, &QTimer::stop);
            connect(handle, &UIShell::BubbleNotificationHandle::hoverExited, timer, [=] { timer->start(); });
            connect(handle, &UIShell::BubbleNotificationHandle::hideClicked, timer, &QObject::deleteLater);
            connect(handle, &UIShell::BubbleNotificationHandle::closeClicked, timer, &QObject::deleteLater);
            connect(timer, &QTimer::timeout, [=] {
                qCInfo(lcNotificationManager) << "Auto-hiding message on timeout:" << message;
                emit handle->hideClicked();
            });
            timer->start();
        }

        const auto removeMessageFromBubbles = [=, this] {
            qCInfo(lcNotificationManager) << "Removing message from bubbles:" << message;
            auto index = m_bubbleMessages.indexOf(message);
            if (index == -1)
                return;
            m_bubbleMessages.removeAt(index);
            emit messageRemovedFromBubbles(index, message);
        };

        const auto removeMessage = [=, this] {
            qCInfo(lcNotificationManager) << "Removing message:" << message;
            removeMessageFromBubbles();
            auto index = m_messages.indexOf(message);
            if (index == -1)
                return;
            bool wasLast = (index == m_messages.size() - 1);
            m_messages.removeAt(index);
            emit messageRemoved(index, message);

            // Update top message title connection if the last message was removed
            if (wasLast) {
                updateTopMessageTitleConnection();
                emit topMessageTitleChanged(topMessageTitle());
            }
        };

        connect(handle, &UIShell::BubbleNotificationHandle::hideClicked, this, removeMessageFromBubbles);
        connect(handle, &UIShell::BubbleNotificationHandle::closeClicked, this, removeMessage);
        connect(handle, &QObject::destroyed, this, removeMessage);

        handle->setTime(QDateTime::currentDateTime());

        if (mode != ProjectWindowInterface::DoNotShowBubble) {
            m_bubbleMessages.append(message);
        }
        m_messages.append(message);

        if (beepOnNotification) {
            QApplication::beep();
        }

        emit messageAdded(m_messages.size() - 1, message);
        if (mode != ProjectWindowInterface::DoNotShowBubble) {
            emit messageAddedToBubbles(m_bubbleMessages.size() - 1, message);
        }

        // Update top message title connection and emit signal since we added a new top message
        updateTopMessageTitleConnection();
        emit topMessageTitleChanged(topMessageTitle());
    }
    QList<NotificationMessage *> NotificationManager::messages() const {
        return m_messages;
    }
    QList<NotificationMessage *> NotificationManager::bubbleMessages() const {
        return m_bubbleMessages;
    }
    QString NotificationManager::topMessageTitle() const {
        if (m_messages.isEmpty()) {
            return QString();
        }
        return m_messages.last()->title();
    }

    void NotificationManager::updateTopMessageTitleConnection() {
        // Disconnect previous connection
        if (m_topMessageTitleConnection) {
            QObject::disconnect(m_topMessageTitleConnection);
            m_topMessageTitleConnection = QMetaObject::Connection();
        }

        // Connect to the new top message's titleChanged signal
        if (!m_messages.isEmpty()) {
            auto topMessage = m_messages.last();
            m_topMessageTitleConnection = connect(topMessage, &NotificationMessage::titleChanged, this, [this](const QString &) {
                emit topMessageTitleChanged(topMessageTitle());
            });
        }
    }
}
