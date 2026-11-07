// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NotificationCenter.h"
#include "NotificationCenter_p.h"

#include <algorithm>

#include <QLoggingCategory>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <notifications/NotificationMessage.h>
#include <notifications/NotificationModel.h>
#include <notifications/private/NotificationEntry_p.h>

namespace Notifications {

    Q_STATIC_LOGGING_CATEGORY(lcNotificationCenter, "diffscope.notifications.center")

    NotificationCenterPrivate::NotificationCenterPrivate(NotificationCenter *q) : q_ptr(q) {
    }

    NotificationCenterPrivate::~NotificationCenterPrivate() {
        shuttingDown = true;
        while (!entries.isEmpty()) {
            removeEntry(entries.last());
        }
    }

    void NotificationCenterPrivate::initialize() {
        Q_Q(NotificationCenter);
        allNotifications = createModel(nullptr, true, false, q);
        globalNotifications = createModel(nullptr, false, false, q);
    }

    NotificationModel *NotificationCenterPrivate::createModel(QObject *scope, bool allScopes, bool bubblesOnly, QObject *parent) {
        return new NotificationModel(this, scope, allScopes, bubblesOnly, parent);
    }

    void NotificationCenterPrivate::publish(NotificationMessage *message, NotificationCenter::NotificationBubbleMode mode, QObject *scope) {
        Q_Q(NotificationCenter);
        if (shuttingDown) {
            return;
        }
        qCInfo(lcNotificationCenter) << "Notification publication requested." << message << scope << mode;
        const auto it = std::ranges::find_if(entries, [message, scope](const NotificationEntry *entry) {
            return entry->message() == message && entry->scope() == scope;
        });
        auto *entry = it == entries.end() ? nullptr : *it;
        if (!entry) {
            entry = new NotificationEntry(message, scope, q);
            entries.append(entry);
            connect(entry, &NotificationEntry::closeRequested, this, [this, entry] {
                removeEntry(entry);
            });
            connect(entry, &NotificationEntry::bubbleChanged, this, [this, entry] {
                Q_EMIT entryChanged(entry);
            });
            connect(entry, &NotificationEntry::contentChanged, this, [this, entry] {
                refreshSuppression(entry);
                if (entry->isActive()) {
                    Q_EMIT entryChanged(entry);
                }
            });
            connect(entry, &NotificationEntry::errorActivated, q, [q, entry] {
                Q_EMIT q->errorActivated(entry->scope());
            });
            connect(entry, &NotificationEntry::doNotShowAgainRequested, q, [q, entry] {
                const auto identifier = entry->message()->doNotShowAgainIdentifier();
                if (!identifier.isEmpty()) {
                    auto identifiers = q->hiddenMessageIdentifiers();
                    identifiers.append(identifier);
                    q->setHiddenMessageIdentifiers(identifiers);
                }
            });
        }

        refreshSuppression(entry);
        if (!entry->isActive()) {
            return;
        }
        const auto identifier = message->doNotShowAgainIdentifier();
        if (!identifier.isEmpty() && hiddenMessageIdentifiers.contains(identifier)) {
            mode = NotificationCenter::DoNotShowBubble;
        }
        if (!entry->publish(mode, ++nextSequence, autoHideTimeout)) {
            return;
        }
        Q_EMIT entryPublished(entry);
        if (!entry->isActive()) {
            return;
        }
        if (message->icon() == SVS::SVSCraft::Critical || message->icon() == SVS::SVSCraft::Warning) {
            Q_EMIT q->errorActivated(scope);
        }
        if (entry->isActive()) {
            Q_EMIT q->notificationPublished(message, scope);
        }
    }

    void NotificationCenterPrivate::removeEntry(NotificationEntry *entry) {
        if (!entries.removeOne(entry)) {
            return;
        }
        entry->deactivate();
        disconnect(entry, nullptr, this, nullptr);
        Q_Q(NotificationCenter);
        disconnect(entry, nullptr, q, nullptr);
        Q_EMIT entryRemoved(entry);
        entry->deleteLater();
    }

    void NotificationCenterPrivate::refreshSuppression(NotificationEntry *entry) {
        if (auto *message = entry->message()) {
            const auto identifier = message->doNotShowAgainIdentifier();
            entry->setSuppressed(!identifier.isEmpty() && hiddenMessageIdentifiers.contains(identifier));
        }
    }

    NotificationCenter::NotificationCenter(QObject *parent) : QObject(parent), d_ptr(new NotificationCenterPrivate(this)) {
        Q_D(NotificationCenter);
        d->initialize();
    }

    NotificationCenter::~NotificationCenter() = default;

    int NotificationCenter::autoHideTimeout() const {
        Q_D(const NotificationCenter);
        return d->autoHideTimeout;
    }

    void NotificationCenter::setAutoHideTimeout(int timeout) {
        Q_D(NotificationCenter);
        timeout = qMax(0, timeout);
        if (d->autoHideTimeout == timeout) {
            return;
        }
        d->autoHideTimeout = timeout;
        Q_EMIT autoHideTimeoutChanged();
    }

    QStringList NotificationCenter::hiddenMessageIdentifiers() const {
        Q_D(const NotificationCenter);
        return d->hiddenMessageIdentifiers;
    }

    void NotificationCenter::setHiddenMessageIdentifiers(const QStringList &identifiers) {
        Q_D(NotificationCenter);
        auto normalizedIdentifiers = identifiers;
        normalizedIdentifiers.removeAll(QString());
        normalizedIdentifiers.removeDuplicates();
        if (d->hiddenMessageIdentifiers == normalizedIdentifiers) {
            return;
        }
        d->hiddenMessageIdentifiers = normalizedIdentifiers;
        const auto entries = d->entries;
        for (auto *entry : entries) {
            if (d->entries.contains(entry)) {
                d->refreshSuppression(entry);
            }
        }
        Q_EMIT hiddenMessageIdentifiersChanged();
    }

    NotificationModel *NotificationCenter::allNotifications() const {
        Q_D(const NotificationCenter);
        return d->allNotifications;
    }

    NotificationModel *NotificationCenter::globalNotifications() const {
        Q_D(const NotificationCenter);
        return d->globalNotifications;
    }

    NotificationModel *NotificationCenter::notificationsForScope(QObject *scope) {
        Q_D(NotificationCenter);
        if (!scope) {
            return d->globalNotifications;
        }
        if (const auto it = d->scopedModels.constFind(scope); it != d->scopedModels.cend()) {
            return *it;
        }
        auto *model = d->createModel(scope, false, false, this);
        d->scopedModels.insert(scope, model);
        connect(scope, &QObject::destroyed, d, [d, scope, model] {
            d->scopedModels.remove(scope);
            model->deleteLater();
        });
        return model;
    }

    void NotificationCenter::sendNotification(NotificationMessage *message, NotificationBubbleMode mode, QObject *scope) {
        Q_D(NotificationCenter);
        d->publish(message, mode, scope);
    }

    void NotificationCenter::clearHiddenMessageIdentifiers() {
        setHiddenMessageIdentifiers({});
    }

}

#include "moc_NotificationCenter.cpp"
#include "moc_NotificationCenter_p.cpp"
