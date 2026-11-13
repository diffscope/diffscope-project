// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONENTRY_P_H
#define DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONENTRY_P_H

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QTimer>

#include <notifications/NotificationCenter.h>
#include <notifications/NotificationMessage.h>

namespace UIShell {
    class BubbleNotificationHandle;
}

namespace Notifications {

    class NotificationEntry : public QObject {
        Q_OBJECT
    public:
        explicit NotificationEntry(NotificationMessage *message, QObject *scope, QObject *parent = nullptr);
        ~NotificationEntry() override;

        NotificationMessage *message() const;
        UIShell::BubbleNotificationHandle *handle() const;
        QObject *scope() const;
        quint64 sequence() const;
        QDateTime publishedAt() const;
        bool isBubble() const;
        bool isActive() const;

        bool publish(NotificationCenter::NotificationBubbleMode mode, quint64 sequence, int autoHideTimeout);
        void setSuppressed(bool suppressed);
        void deactivate();

    Q_SIGNALS:
        void bubbleChanged();
        void contentChanged();
        void closeRequested();
        void doNotShowAgainRequested();

    private:
        void hide();
        void updatePermanentlyHideable();

        QPointer<NotificationMessage> m_message;
        UIShell::BubbleNotificationHandle *m_handle{};
        QObject *m_scope{};
        QList<QMetaObject::Connection> m_connections;
        QTimer m_autoHideTimer;
        quint64 m_sequence{};
        QDateTime m_publishedAt;
        int m_hoverCount{};
        bool m_isBubble{};
        bool m_autoHide{};
        bool m_suppressed{};
        bool m_active{true};
    };

}

#endif // DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONENTRY_P_H
