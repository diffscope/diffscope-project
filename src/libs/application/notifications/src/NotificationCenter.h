// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONCENTER_H
#define DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONCENTER_H

#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <qqmlintegration.h>

#include <notifications/notificationsglobal.h>

namespace Notifications {

    class NotificationCenterPrivate;
    class NotificationMessage;
    class NotificationModel;

    class NOTIFICATIONS_EXPORT NotificationCenter : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(NotificationCenter)
        Q_PROPERTY(int autoHideTimeout READ autoHideTimeout WRITE setAutoHideTimeout NOTIFY autoHideTimeoutChanged)
        Q_PROPERTY(QStringList hiddenMessageIdentifiers READ hiddenMessageIdentifiers WRITE setHiddenMessageIdentifiers NOTIFY hiddenMessageIdentifiersChanged)
        Q_PROPERTY(NotificationModel *allNotifications READ allNotifications CONSTANT)
        Q_PROPERTY(NotificationModel *globalNotifications READ globalNotifications CONSTANT)

    public:
        enum NotificationBubbleMode {
            NormalBubble,
            DoNotShowBubble,
            AutoHide,
        };
        Q_ENUM(NotificationBubbleMode)

        explicit NotificationCenter(QObject *parent = nullptr);
        ~NotificationCenter() override;

        int autoHideTimeout() const;
        void setAutoHideTimeout(int timeout);

        QStringList hiddenMessageIdentifiers() const;
        void setHiddenMessageIdentifiers(const QStringList &identifiers);

        NotificationModel *allNotifications() const;
        NotificationModel *globalNotifications() const;
        // Returns global notifications if scope is null, or global notifications and notifications for scope otherwise.
        // The scoped model is destroyed after scope is destroyed.
        Q_INVOKABLE NotificationModel *notificationsForScope(QObject *scope);

        Q_INVOKABLE void sendNotification(NotificationMessage *message, NotificationBubbleMode mode = NormalBubble, QObject *scope = nullptr);
        Q_INVOKABLE void clearHiddenMessageIdentifiers();

    Q_SIGNALS:
        void autoHideTimeoutChanged();
        void hiddenMessageIdentifiersChanged();
        void notificationPublished(Notifications::NotificationMessage *message, QObject *scope);

    private:
        QScopedPointer<NotificationCenterPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONCENTER_H
