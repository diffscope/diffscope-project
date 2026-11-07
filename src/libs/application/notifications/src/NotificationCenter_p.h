// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONCENTER_P_H
#define DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONCENTER_P_H

#include <notifications/NotificationCenter.h>

#include <QHash>
#include <QList>

namespace Notifications {

    class NotificationEntry;

    class NotificationCenterPrivate : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(NotificationCenter)
    public:
        static NotificationCenterPrivate *get(NotificationCenter *center) {
            return center->d_func();
        }

        explicit NotificationCenterPrivate(NotificationCenter *q);
        ~NotificationCenterPrivate() override;

        void initialize();
        NotificationModel *createModel(QObject *scope, bool allScopes, bool bubblesOnly, QObject *parent);
        void publish(NotificationMessage *message, NotificationCenter::NotificationBubbleMode mode, QObject *scope);
        void removeEntry(NotificationEntry *entry);
        void refreshSuppression(NotificationEntry *entry);

        NotificationCenter *q_ptr{};
        QList<NotificationEntry *> entries;
        NotificationModel *allNotifications{};
        NotificationModel *globalNotifications{};
        QHash<QObject *, NotificationModel *> scopedModels;
        QStringList hiddenMessageIdentifiers;
        int autoHideTimeout{5000};
        quint64 nextSequence{};
        bool shuttingDown{};

    Q_SIGNALS:
        void entryPublished(Notifications::NotificationEntry *entry);
        void entryChanged(Notifications::NotificationEntry *entry);
        void entryRemoved(Notifications::NotificationEntry *entry);
    };

}

#endif // DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONCENTER_P_H
