// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONPRESENTATIONMODEL_P_H
#define DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONPRESENTATIONMODEL_P_H

#include <QIdentityProxyModel>
#include <qqmlintegration.h>

#include <notifications/NotificationModel.h>

namespace Notifications {

    class NotificationPresentationModel : public QIdentityProxyModel {
        Q_OBJECT
        QML_NAMED_ELEMENT(NotificationPresentationModel)
        Q_PROPERTY(Notifications::NotificationModel *notifications READ notifications WRITE setNotifications NOTIFY notificationsChanged)

    public:
        explicit NotificationPresentationModel(QObject *parent = nullptr);

        NotificationModel *notifications() const;
        void setNotifications(NotificationModel *model);

        QVariant data(const QModelIndex &index, int role) const override;
        QHash<int, QByteArray> roleNames() const override;

    Q_SIGNALS:
        void notificationsChanged();

    private:
        enum {
            HandleRole = NotificationModel::PublishedAtRole + 1,
        };
    };

}

#endif // DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONPRESENTATIONMODEL_P_H
