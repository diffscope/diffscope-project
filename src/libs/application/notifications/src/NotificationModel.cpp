// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NotificationModel.h"
#include "NotificationModel_p.h"

#include <algorithm>

#include <QPointer>
#include <QSet>

#include <notifications/NotificationMessage.h>
#include <notifications/private/NotificationCenter_p.h>
#include <notifications/private/NotificationEntry_p.h>

namespace Notifications {

    void NotificationModelPrivate::initialize(NotificationCenterPrivate *center) {
        Q_Q(NotificationModel);
        for (auto *entry : center->entries) {
            if (accepts(entry)) {
                entries.append(entry);
            }
        }
        std::ranges::sort(entries, [this](const NotificationEntry *left, const NotificationEntry *right) {
            return bubblesOnly ? left->sequence() < right->sequence() : left->sequence() > right->sequence();
        });
        QObject::connect(center, &NotificationCenterPrivate::entryPublished, q, [this](NotificationEntry *entry) {
            synchronize(entry, true);
        });
        QObject::connect(center, &NotificationCenterPrivate::entryChanged, q, [this](NotificationEntry *entry) {
            synchronize(entry, false);
        });
        QObject::connect(center, &NotificationCenterPrivate::entryRemoved, q, [this](NotificationEntry *entry) {
            removeEntry(entry);
        });
        bubbleModel = bubblesOnly ? q : center->createModel(scope, allScopes, true, q);
    }

    bool NotificationModelPrivate::accepts(NotificationEntry *entry) const {
        return entry->isActive() && (allScopes || !entry->scope() || entry->scope() == scope) && (!bubblesOnly || entry->isBubble());
    }

    void NotificationModelPrivate::synchronize(NotificationEntry *entry, bool published) {
        Q_Q(NotificationModel);
        const auto oldRow = entries.indexOf(entry);
        if (!accepts(entry)) {
            removeEntry(entry);
            return;
        }
        const auto targetRow = static_cast<int>(std::ranges::count_if(entries, [this, entry](const NotificationEntry *existing) {
            return existing != entry && (bubblesOnly ? existing->sequence() < entry->sequence() : existing->sequence() > entry->sequence());
        }));
        if (oldRow < 0) {
            q->beginInsertRows({}, targetRow, targetRow);
            entries.insert(targetRow, entry);
            q->endInsertRows();
            Q_EMIT q->countChanged();
            return;
        }
        if (published) {
            const auto destination = targetRow > oldRow ? targetRow + 1 : targetRow;
            if (oldRow != targetRow && q->beginMoveRows({}, static_cast<int>(oldRow), static_cast<int>(oldRow), {}, destination)) {
                entries.move(oldRow, targetRow);
                q->endMoveRows();
            }
        }
        const auto row = entries.indexOf(entry);
        if (row >= 0) {
            const auto modelIndex = q->index(static_cast<int>(row), 0);
            Q_EMIT q->dataChanged(modelIndex, modelIndex);
        }
    }

    void NotificationModelPrivate::removeEntry(NotificationEntry *entry) {
        Q_Q(NotificationModel);
        const auto row = entries.indexOf(entry);
        if (row < 0) {
            return;
        }
        q->beginRemoveRows({}, static_cast<int>(row), static_cast<int>(row));
        entries.removeAt(row);
        q->endRemoveRows();
        Q_EMIT q->countChanged();
    }

    NotificationEntry *NotificationModelPrivate::entryAt(int row) const {
        return entries.at(row);
    }

    NotificationModel::NotificationModel(NotificationCenterPrivate *center, QObject *scope, bool allScopes, bool bubblesOnly, QObject *parent)
        : QAbstractListModel(parent), d_ptr(new NotificationModelPrivate) {
        Q_D(NotificationModel);
        d->q_ptr = this;
        d->scope = scope;
        d->allScopes = allScopes;
        d->bubblesOnly = bubblesOnly;
        d->initialize(center);
    }

    NotificationModel::~NotificationModel() = default;

    int NotificationModel::rowCount(const QModelIndex &parent) const {
        Q_D(const NotificationModel);
        return parent.isValid() ? 0 : static_cast<int>(d->entries.size());
    }

    QVariant NotificationModel::data(const QModelIndex &index, int role) const {
        Q_D(const NotificationModel);
        if (!index.isValid() || index.model() != this || index.column() != 0 || index.row() < 0 || index.row() >= rowCount()) {
            return {};
        }
        const auto *entry = d->entryAt(index.row());
        switch (role) {
            case Qt::DisplayRole:
                return entry->message() ? entry->message()->title() : QString();
            case MessageRole:
                return QVariant::fromValue(entry->message());
            case ScopeRole:
                return QVariant::fromValue(entry->scope());
            case BubbleRole:
                return entry->isBubble();
            case PublishedAtRole:
                return entry->publishedAt();
            default:
                return {};
        }
    }

    QHash<int, QByteArray> NotificationModel::roleNames() const {
        return {
            {Qt::DisplayRole, "display"},
            {MessageRole, "notificationMessage"},
            {ScopeRole, "scope"},
            {BubbleRole, "bubble"},
            {PublishedAtRole, "publishedAt"},
        };
    }

    Qt::ItemFlags NotificationModel::flags(const QModelIndex &index) const {
        return index.isValid() && index.model() == this && index.row() >= 0 && index.row() < rowCount() && index.column() == 0 ? Qt::ItemIsEnabled | Qt::ItemIsSelectable : Qt::NoItemFlags;
    }

    int NotificationModel::count() const {
        return rowCount();
    }

    NotificationMessage *NotificationModel::messageAt(int row) const {
        Q_D(const NotificationModel);
        if (row < 0 || row >= rowCount()) {
            return nullptr;
        }
        return d->entryAt(row)->message();
    }

    NotificationModel *NotificationModel::bubbleModel() const {
        Q_D(const NotificationModel);
        return d->bubbleModel;
    }

    void NotificationModel::closeAll() {
        Q_D(NotificationModel);
        QList<QPointer<NotificationMessage>> messages;
        QSet<NotificationMessage *> seen;
        for (const auto *entry : d->entries) {
            if (auto *message = entry->message(); message && !seen.contains(message)) {
                seen.insert(message);
                messages.append(message);
            }
        }
        for (const auto &message : messages) {
            if (message) {
                message->close();
            }
        }
    }

}

#include "moc_NotificationModel.cpp"
