// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NotificationPresentationModel_p.h"

#include <uishell/BubbleNotificationHandle.h>

#include <notifications/private/NotificationEntry_p.h>
#include <notifications/private/NotificationModel_p.h>

namespace Notifications {

    NotificationPresentationModel::NotificationPresentationModel(QObject *parent) : QIdentityProxyModel(parent) {
        connect(this, &QAbstractProxyModel::sourceModelChanged, this, &NotificationPresentationModel::notificationsChanged);
    }

    NotificationModel *NotificationPresentationModel::notifications() const {
        return static_cast<NotificationModel *>(sourceModel());
    }

    void NotificationPresentationModel::setNotifications(NotificationModel *model) {
        setSourceModel(model);
    }

    QVariant NotificationPresentationModel::data(const QModelIndex &index, int role) const {
        if (role != HandleRole) {
            return QIdentityProxyModel::data(index, role);
        }
        if (!index.isValid() || index.model() != this || index.column() != 0 || index.row() < 0 || index.row() >= rowCount()) {
            return {};
        }
        const auto *entry = NotificationModelPrivate::get(notifications())->entryAt(index.row());
        return QVariant::fromValue(entry->handle());
    }

    QHash<int, QByteArray> NotificationPresentationModel::roleNames() const {
        auto roles = QIdentityProxyModel::roleNames();
        roles.insert(HandleRole, "notificationHandle");
        return roles;
    }

}

#include "moc_NotificationPresentationModel_p.cpp"
