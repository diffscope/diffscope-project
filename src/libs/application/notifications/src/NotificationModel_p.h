// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONMODEL_P_H
#define DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONMODEL_P_H

#include <notifications/NotificationModel.h>

#include <QList>

namespace Notifications {

    class NotificationEntry;

    class NotificationModelPrivate {
        Q_DECLARE_PUBLIC(NotificationModel)
    public:
        static NotificationModelPrivate *get(NotificationModel *model) {
            return model->d_func();
        }

        void initialize(NotificationCenterPrivate *center);
        bool accepts(NotificationEntry *entry) const;
        void synchronize(NotificationEntry *entry, bool published);
        void removeEntry(NotificationEntry *entry);
        NotificationEntry *entryAt(int row) const;

        NotificationModel *q_ptr{};
        QObject *scope{};
        bool allScopes{};
        bool bubblesOnly{};
        NotificationModel *bubbleModel{};
        QList<NotificationEntry *> entries;
    };

}

#endif // DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONMODEL_P_H
