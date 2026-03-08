#include "NotificationManager.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QSettings>
#include <QTimer>

#include <CoreApi/runtimeinterface.h>
#include <SVSCraftCore/SVSCraftNamespace.h>

#include <uishell/BubbleNotificationHandle.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/NotificationMessage.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcNotificationManager, "diffscope.core.notificationmanager")

    NotificationManager::NotificationManager(ProjectWindowInterface *parent) : QObject(parent) {
        parent->setProperty(staticMetaObject.className(), QVariant::fromValue(this));
        loadHiddenMessageIdentifiers();

        // Listen to reset all do not show again requests
        connect(CoreInterface::instance(), &CoreInterface::resetAllDoNotShowAgainRequested, this, [this] {
            clearHiddenMessageIdentifiers();
        });
    }
    NotificationManager::~NotificationManager() = default;
    NotificationManager *NotificationManager::of(ProjectWindowInterface *windowHandle) {
        return windowHandle->property(staticMetaObject.className()).value<NotificationManager *>();
    }

    void NotificationManager::addMessage(NotificationMessage *message, ProjectWindowInterface::NotificationBubbleMode mode) {
        qCInfo(lcNotificationManager) << "Adding message:" << message << message->title() << message->text() << mode;

        // Check if this message should be hidden based on its identifier
        QString identifier = message->doNotShowAgainIdentifier();
        if (!identifier.isEmpty() && isMessageHidden(identifier)) {
            qCInfo(lcNotificationManager) << "Message identifier" << identifier << "is hidden, using DoNotShowBubble mode";
            mode = ProjectWindowInterface::DoNotShowBubble;
            message->setAllowDoNotShowAgain(false);
        }

        // Listen to doNotShowAgainRequested signal
        connect(message, &NotificationMessage::doNotShowAgainRequested, this, [this, message] {
            QString identifier = message->doNotShowAgainIdentifier();
            if (!identifier.isEmpty() && !m_hiddenMessageIdentifiers.contains(identifier)) {
                qCInfo(lcNotificationManager) << "Adding message identifier to hidden list:" << identifier;
                m_hiddenMessageIdentifiers.append(identifier);
                saveHiddenMessageIdentifiers();
                message->setAllowDoNotShowAgain(false);
            }
        });

        // Listen to iconChanged signal to update counts
        connect(message, &NotificationMessage::iconChanged, this,
            [this, message](SVS::SVSCraft::MessageBoxIcon newIcon) {
                auto oldIcon = m_messageIcons.value(message, SVS::SVSCraft::NoIcon);
                updateIconCount(oldIcon, newIcon);
                m_messageIcons[message] = newIcon;
                
                // Emit errorActivated if icon changed to Critical or Warning
                bool wasError = (oldIcon == SVS::SVSCraft::Critical || oldIcon == SVS::SVSCraft::Warning);
                bool isError = (newIcon == SVS::SVSCraft::Critical || newIcon == SVS::SVSCraft::Warning);
                if (!wasError && isError) {
                    emit errorActivated();
                }
            });

        // Initialize icon count based on current icon
        auto currentIcon = message->icon();
        m_messageIcons[message] = currentIcon;
        if (currentIcon == SVS::SVSCraft::Critical) {
            m_criticalCount++;
            emit criticalCountChanged(m_criticalCount);
            emit errorActivated();
        } else if (currentIcon == SVS::SVSCraft::Warning) {
            m_warningCount++;
            emit warningCountChanged(m_warningCount);
            emit errorActivated();
        }

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
            connect(timer, &QTimer::timeout, message, [=] {
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

            // Disconnect all signals for this message and update icon count
            // Use hash to get icon instead of calling message->icon() as message may be being destroyed
            auto currentIcon = m_messageIcons.value(message, SVS::SVSCraft::NoIcon);
            if (currentIcon == SVS::SVSCraft::Critical) {
                m_criticalCount--;
                emit criticalCountChanged(m_criticalCount);
            } else if (currentIcon == SVS::SVSCraft::Warning) {
                m_warningCount--;
                emit warningCountChanged(m_warningCount);
            }
            disconnectMessageSignals(message);

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

    int NotificationManager::criticalCount() const {
        return m_criticalCount;
    }

    int NotificationManager::warningCount() const {
        return m_warningCount;
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

    void NotificationManager::loadHiddenMessageIdentifiers() {
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        m_hiddenMessageIdentifiers = settings->value("hiddenMessageIdentifiers", QStringList()).toStringList();
        settings->endGroup();
        qCInfo(lcNotificationManager) << "Loaded hidden message identifiers:" << m_hiddenMessageIdentifiers;
    }

    void NotificationManager::saveHiddenMessageIdentifiers() {
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue("hiddenMessageIdentifiers", m_hiddenMessageIdentifiers);
        settings->endGroup();
        qCInfo(lcNotificationManager) << "Saved hidden message identifiers:" << m_hiddenMessageIdentifiers;
    }

    bool NotificationManager::isMessageHidden(const QString &identifier) const {
        return m_hiddenMessageIdentifiers.contains(identifier);
    }

    void NotificationManager::clearHiddenMessageIdentifiers() {
        qCInfo(lcNotificationManager) << "Clearing hidden message identifiers";
        m_hiddenMessageIdentifiers.clear();
        saveHiddenMessageIdentifiers();

        // Reset allowDoNotShowAgain for all messages with non-empty doNotShowAgainIdentifier
        for (auto *message : m_messages) {
            if (!message->doNotShowAgainIdentifier().isEmpty()) {
                message->setAllowDoNotShowAgain(true);
            }
        }
    }

    void NotificationManager::updateIconCount(SVS::SVSCraft::MessageBoxIcon oldIcon, SVS::SVSCraft::MessageBoxIcon newIcon) {
        // Decrement old icon count
        if (oldIcon == SVS::SVSCraft::Critical) {
            m_criticalCount--;
            emit criticalCountChanged(m_criticalCount);
        } else if (oldIcon == SVS::SVSCraft::Warning) {
            m_warningCount--;
            emit warningCountChanged(m_warningCount);
        }

        // Increment new icon count
        if (newIcon == SVS::SVSCraft::Critical) {
            m_criticalCount++;
            emit criticalCountChanged(m_criticalCount);
        } else if (newIcon == SVS::SVSCraft::Warning) {
            m_warningCount++;
            emit warningCountChanged(m_warningCount);
        }
    }

    void NotificationManager::disconnectMessageSignals(NotificationMessage *message) {
        // Disconnect all signal connections from message to this
        disconnect(message, nullptr, this, nullptr);
        
        // Disconnect all signal connections from handle to this
        auto handle = message->property("handle").value<UIShell::BubbleNotificationHandle *>();
        if (handle) {
            disconnect(handle, nullptr, this, nullptr);
        }
        
        // Remove icon tracking for this message
        m_messageIcons.remove(message);
    }
}
