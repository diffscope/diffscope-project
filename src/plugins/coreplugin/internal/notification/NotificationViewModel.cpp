#include "NotificationViewModel.h"

#include <algorithm>

#include <coreplugin/NotificationMessage.h>
#include <coreplugin/internal/NotificationManager.h>

namespace Core::Internal {

    NotificationViewModel::NotificationViewModel(const QList<NotificationManager *> &notificationManagers, QObject *parent)
        : QObject(parent), m_notificationManagers(notificationManagers) {
        for (auto *notificationManager : m_notificationManagers) {
            for (auto *message : notificationManager->messages()) {
                insertMessage(m_messages, message);
            }
            for (auto *message : notificationManager->bubbleMessages()) {
                insertMessage(m_bubbleMessages, message);
            }

            connect(notificationManager, &NotificationManager::messageAdded, this, [this](int, NotificationMessage *message) {
                const auto index = insertMessage(m_messages, message);
                emit messageAdded(index, message);
                updateTopMessageTitleConnection();
                emit topMessageTitleChanged(topMessageTitle());
            });
            connect(notificationManager, &NotificationManager::messageRemoved, this, [this](int, NotificationMessage *message) {
                const auto index = m_messages.indexOf(message);
                if (index < 0) {
                    return;
                }
                m_messages.removeAt(index);
                emit messageRemoved(index, message);
                updateTopMessageTitleConnection();
                emit topMessageTitleChanged(topMessageTitle());
            });
            connect(notificationManager, &NotificationManager::messageAddedToBubbles, this, [this](int, NotificationMessage *message) {
                const auto index = insertMessage(m_bubbleMessages, message);
                emit messageAddedToBubbles(index, message);
            });
            connect(notificationManager, &NotificationManager::messageRemovedFromBubbles, this, [this](int, NotificationMessage *message) {
                const auto index = m_bubbleMessages.indexOf(message);
                if (index < 0) {
                    return;
                }
                m_bubbleMessages.removeAt(index);
                emit messageRemovedFromBubbles(index, message);
            });
            connect(notificationManager, &NotificationManager::criticalCountChanged, this, [this] {
                emit criticalCountChanged(criticalCount());
            });
            connect(notificationManager, &NotificationManager::warningCountChanged, this, [this] {
                emit warningCountChanged(warningCount());
            });
            connect(notificationManager, &NotificationManager::errorActivated, this, &NotificationViewModel::errorActivated);
        }
        updateTopMessageTitleConnection();
    }

    NotificationViewModel::~NotificationViewModel() = default;

    QList<NotificationMessage *> NotificationViewModel::messages() const {
        return m_messages;
    }

    QList<NotificationMessage *> NotificationViewModel::bubbleMessages() const {
        return m_bubbleMessages;
    }

    QString NotificationViewModel::topMessageTitle() const {
        return m_messages.isEmpty() ? QString() : m_messages.last()->title();
    }

    int NotificationViewModel::criticalCount() const {
        int count = 0;
        for (const auto *notificationManager : m_notificationManagers) {
            count += notificationManager->criticalCount();
        }
        return count;
    }

    int NotificationViewModel::warningCount() const {
        int count = 0;
        for (const auto *notificationManager : m_notificationManagers) {
            count += notificationManager->warningCount();
        }
        return count;
    }

    quint64 NotificationViewModel::messageSequence(NotificationMessage *message) const {
        for (const auto *notificationManager : m_notificationManagers) {
            const auto sequence = notificationManager->messageSequence(message);
            if (sequence != 0) {
                return sequence;
            }
        }
        return 0;
    }

    int NotificationViewModel::insertMessage(QList<NotificationMessage *> &messages, NotificationMessage *message) {
        const auto sequence = messageSequence(message);
        const auto it = std::lower_bound(messages.cbegin(), messages.cend(), sequence, [this](NotificationMessage *existingMessage, quint64 targetSequence) {
            return messageSequence(existingMessage) < targetSequence;
        });
        const auto index = static_cast<int>(std::distance(messages.cbegin(), it));
        messages.insert(index, message);
        return index;
    }

    void NotificationViewModel::updateTopMessageTitleConnection() {
        if (m_topMessageTitleConnection) {
            disconnect(m_topMessageTitleConnection);
            m_topMessageTitleConnection = {};
        }
        if (!m_messages.isEmpty()) {
            m_topMessageTitleConnection = connect(m_messages.last(), &NotificationMessage::titleChanged, this, [this] {
                emit topMessageTitleChanged(topMessageTitle());
            });
        }
    }

}

#include "moc_NotificationViewModel.cpp"
