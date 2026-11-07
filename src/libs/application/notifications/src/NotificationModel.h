// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONMODEL_H
#define DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONMODEL_H

#include <QAbstractListModel>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <notifications/notificationsglobal.h>

namespace Notifications {

    class NotificationCenterPrivate;
    class NotificationMessage;
    class NotificationModelPrivate;

    class NOTIFICATIONS_EXPORT NotificationModel : public QAbstractListModel {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(NotificationModel)
        Q_PROPERTY(int count READ count NOTIFY countChanged)
        Q_PROPERTY(NotificationModel *bubbleModel READ bubbleModel CONSTANT)

    public:
        enum Role {
            MessageRole = Qt::UserRole + 1,
            ScopeRole,
            BubbleRole,
            PublishedAtRole,
        };
        Q_ENUM(Role)

        ~NotificationModel() override;

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        QHash<int, QByteArray> roleNames() const override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;

        int count() const;
        // Returns the message at row, or nullptr if row is invalid.
        Q_INVOKABLE NotificationMessage *messageAt(int row) const;
        // Returns the bubble subset, or this model if it already contains only bubbles.
        NotificationModel *bubbleModel() const;

        Q_INVOKABLE void closeAll();

    Q_SIGNALS:
        void countChanged();

    private:
        explicit NotificationModel(NotificationCenterPrivate *center, QObject *scope, bool allScopes, bool bubblesOnly, QObject *parent);
        QScopedPointer<NotificationModelPrivate> d_ptr;

        friend class NotificationCenterPrivate;
    };

}

#endif // DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONMODEL_H
