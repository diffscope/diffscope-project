// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NotificationEntry_p.h"

#include <uishell/BubbleNotificationHandle.h>

namespace Notifications {

    NotificationEntry::NotificationEntry(NotificationMessage *message, QObject *scope, QObject *parent)
        : QObject(parent), m_message(message), m_handle(new UIShell::BubbleNotificationHandle(this)), m_scope(scope) {
        m_handle->setTitle(message->title());
        m_connections.append(connect(message, &NotificationMessage::titleChanged, m_handle, &UIShell::BubbleNotificationHandle::setTitle));
        m_handle->setText(message->text());
        m_connections.append(connect(message, &NotificationMessage::textChanged, m_handle, &UIShell::BubbleNotificationHandle::setText));
        m_handle->setIcon(message->icon());
        m_connections.append(connect(message, &NotificationMessage::iconChanged, m_handle, &UIShell::BubbleNotificationHandle::setIcon));
        m_handle->setButtons(message->buttons());
        m_connections.append(connect(message, &NotificationMessage::buttonsChanged, m_handle, &UIShell::BubbleNotificationHandle::setButtons));
        m_handle->setPrimaryButton(message->primaryButton());
        m_connections.append(connect(message, &NotificationMessage::primaryButtonChanged, m_handle, &UIShell::BubbleNotificationHandle::setPrimaryButton));
        m_handle->setClosable(message->closable());
        m_connections.append(connect(message, &NotificationMessage::closableChanged, m_handle, &UIShell::BubbleNotificationHandle::setClosable));
        m_handle->setHasProgress(message->hasProgress());
        m_connections.append(connect(message, &NotificationMessage::hasProgressChanged, m_handle, &UIShell::BubbleNotificationHandle::setHasProgress));
        m_handle->setProgress(message->progress());
        m_connections.append(connect(message, &NotificationMessage::progressChanged, m_handle, &UIShell::BubbleNotificationHandle::setProgress));
        m_handle->setProgressAbortable(message->progressAbortable());
        m_connections.append(connect(message, &NotificationMessage::progressAbortableChanged, m_handle, &UIShell::BubbleNotificationHandle::setProgressAbortable));
        m_handle->setTextFormat(message->textFormat());
        m_connections.append(connect(message, &NotificationMessage::textFormatChanged, m_handle, &UIShell::BubbleNotificationHandle::setTextFormat));
        updatePermanentlyHideable();
        m_connections.append(connect(message, &NotificationMessage::allowDoNotShowAgainChanged, this, &NotificationEntry::updatePermanentlyHideable));
        m_connections.append(connect(message, &NotificationMessage::titleChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::textChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::iconChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::buttonsChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::primaryButtonChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::closableChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::hasProgressChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::progressChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::progressAbortableChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::textFormatChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::allowDoNotShowAgainChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::doNotShowAgainIdentifierChanged, this, &NotificationEntry::contentChanged));
        m_connections.append(connect(message, &NotificationMessage::hidden, this, &NotificationEntry::hide));
        m_connections.append(connect(message, &NotificationMessage::closed, this, &NotificationEntry::closeRequested));
        m_connections.append(connect(message, &NotificationMessage::doNotShowAgainRequested, this, &NotificationEntry::doNotShowAgainRequested));
        m_connections.append(connect(message, &QObject::destroyed, this, &NotificationEntry::closeRequested));
        m_connections.append(connect(m_handle, &UIShell::BubbleNotificationHandle::hideClicked, message, &NotificationMessage::hide));
        m_connections.append(connect(m_handle, &UIShell::BubbleNotificationHandle::closeClicked, message, &NotificationMessage::close));
        m_connections.append(connect(m_handle, &UIShell::BubbleNotificationHandle::abortClicked, message, &NotificationMessage::progressAborted));
        m_connections.append(connect(m_handle, &UIShell::BubbleNotificationHandle::permanentlyHideClicked, message, &NotificationMessage::doNotShowAgainRequested));
        m_connections.append(connect(m_handle, &UIShell::BubbleNotificationHandle::buttonClicked, message, &NotificationMessage::buttonClicked));
        m_connections.append(connect(m_handle, &UIShell::BubbleNotificationHandle::linkActivated, message, &NotificationMessage::linkActivated));
        m_connections.append(connect(m_handle, &UIShell::BubbleNotificationHandle::hoverEntered, this, [this] {
            ++m_hoverCount;
            m_autoHideTimer.stop();
        }));
        m_connections.append(connect(m_handle, &UIShell::BubbleNotificationHandle::hoverExited, this, [this] {
            m_hoverCount = qMax(0, m_hoverCount - 1);
            if (m_hoverCount == 0 && m_isBubble && m_autoHide && m_active) {
                m_autoHideTimer.start();
            }
        }));
        m_autoHideTimer.setSingleShot(true);
        connect(&m_autoHideTimer, &QTimer::timeout, this, [this] {
            if (m_active) {
                m_message->hide();
            }
        });
        if (scope) {
            m_connections.append(connect(scope, &QObject::destroyed, this, &NotificationEntry::closeRequested));
        }
    }

    NotificationEntry::~NotificationEntry() {
        deactivate();
    }

    NotificationMessage *NotificationEntry::message() const {
        return m_message;
    }

    UIShell::BubbleNotificationHandle *NotificationEntry::handle() const {
        return m_handle;
    }

    QObject *NotificationEntry::scope() const {
        return m_scope;
    }

    quint64 NotificationEntry::sequence() const {
        return m_sequence;
    }

    QDateTime NotificationEntry::publishedAt() const {
        return m_publishedAt;
    }

    bool NotificationEntry::isBubble() const {
        return m_isBubble;
    }

    bool NotificationEntry::isActive() const {
        return m_active;
    }

    bool NotificationEntry::publish(NotificationCenter::NotificationBubbleMode mode, quint64 sequence, int autoHideTimeout) {
        m_autoHideTimer.stop();
        m_autoHideTimer.setInterval(autoHideTimeout);
        m_sequence = sequence;
        m_publishedAt = QDateTime::currentDateTime();
        m_autoHide = mode == NotificationCenter::AutoHide;
        m_isBubble = mode != NotificationCenter::DoNotShowBubble;
        m_handle->setTime(m_publishedAt);
        if (!m_active) {
            return false;
        }
        if (m_isBubble && m_autoHide && m_hoverCount == 0) {
            m_autoHideTimer.start();
        }
        return true;
    }

    void NotificationEntry::setSuppressed(bool suppressed) {
        m_suppressed = suppressed;
        updatePermanentlyHideable();
    }

    void NotificationEntry::deactivate() {
        m_active = false;
        m_autoHideTimer.stop();
        for (const auto &connection : m_connections) {
            disconnect(connection);
        }
        m_connections.clear();
    }

    void NotificationEntry::hide() {
        m_autoHideTimer.stop();
        if (!m_isBubble) {
            return;
        }
        m_isBubble = false;
        Q_EMIT bubbleChanged();
    }

    void NotificationEntry::updatePermanentlyHideable() {
        m_handle->setPermanentlyHideable(m_message->allowDoNotShowAgain() && !m_suppressed);
    }

}

#include "moc_NotificationEntry_p.cpp"
