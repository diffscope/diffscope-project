// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NotificationMessage.h"
#include "NotificationMessage_p.h"

namespace Notifications {

    NotificationMessage::NotificationMessage(QObject *parent) : QObject(parent), d_ptr(new NotificationMessagePrivate) {
        Q_D(NotificationMessage);
        d->q_ptr = this;
    }

    NotificationMessage::~NotificationMessage() = default;

    QString NotificationMessage::title() const {
        Q_D(const NotificationMessage);
        return d->title;
    }

    void NotificationMessage::setTitle(const QString &title) {
        Q_D(NotificationMessage);
        if (d->title == title) {
            return;
        }
        d->title = title;
        Q_EMIT titleChanged(title);
    }

    QString NotificationMessage::text() const {
        Q_D(const NotificationMessage);
        return d->text;
    }

    void NotificationMessage::setText(const QString &text) {
        Q_D(NotificationMessage);
        if (d->text == text) {
            return;
        }
        d->text = text;
        Q_EMIT textChanged(text);
    }

    SVS::SVSCraft::MessageBoxIcon NotificationMessage::icon() const {
        Q_D(const NotificationMessage);
        return d->icon;
    }

    void NotificationMessage::setIcon(SVS::SVSCraft::MessageBoxIcon icon) {
        Q_D(NotificationMessage);
        if (d->icon == icon) {
            return;
        }
        d->icon = icon;
        Q_EMIT iconChanged(icon);
    }

    QStringList NotificationMessage::buttons() const {
        Q_D(const NotificationMessage);
        return d->buttons;
    }

    void NotificationMessage::setButtons(const QStringList &buttons) {
        Q_D(NotificationMessage);
        if (d->buttons == buttons) {
            return;
        }
        d->buttons = buttons;
        Q_EMIT buttonsChanged(buttons);
    }

    int NotificationMessage::primaryButton() const {
        Q_D(const NotificationMessage);
        return d->primaryButton;
    }

    void NotificationMessage::setPrimaryButton(int primaryButton) {
        Q_D(NotificationMessage);
        if (d->primaryButton == primaryButton) {
            return;
        }
        d->primaryButton = primaryButton;
        Q_EMIT primaryButtonChanged(primaryButton);
    }

    bool NotificationMessage::closable() const {
        Q_D(const NotificationMessage);
        return d->closable;
    }

    void NotificationMessage::setClosable(bool closable) {
        Q_D(NotificationMessage);
        if (d->closable == closable) {
            return;
        }
        d->closable = closable;
        Q_EMIT closableChanged(closable);
    }

    bool NotificationMessage::hasProgress() const {
        Q_D(const NotificationMessage);
        return d->hasProgress;
    }

    void NotificationMessage::setHasProgress(bool hasProgress) {
        Q_D(NotificationMessage);
        if (d->hasProgress == hasProgress) {
            return;
        }
        d->hasProgress = hasProgress;
        Q_EMIT hasProgressChanged(hasProgress);
    }

    double NotificationMessage::progress() const {
        Q_D(const NotificationMessage);
        return d->progress;
    }

    void NotificationMessage::setProgress(double progress) {
        Q_D(NotificationMessage);
        if (qFuzzyCompare(d->progress, progress)) {
            return;
        }
        d->progress = progress;
        Q_EMIT progressChanged(progress);
    }

    bool NotificationMessage::progressAbortable() const {
        Q_D(const NotificationMessage);
        return d->progressAbortable;
    }

    void NotificationMessage::setProgressAbortable(bool progressAbortable) {
        Q_D(NotificationMessage);
        if (d->progressAbortable == progressAbortable) {
            return;
        }
        d->progressAbortable = progressAbortable;
        Q_EMIT progressAbortableChanged(progressAbortable);
    }

    bool NotificationMessage::allowDoNotShowAgain() const {
        Q_D(const NotificationMessage);
        return d->allowDoNotShowAgain;
    }

    void NotificationMessage::setAllowDoNotShowAgain(bool allowDoNotShowAgain) {
        Q_D(NotificationMessage);
        if (d->allowDoNotShowAgain == allowDoNotShowAgain) {
            return;
        }
        d->allowDoNotShowAgain = allowDoNotShowAgain;
        Q_EMIT allowDoNotShowAgainChanged(allowDoNotShowAgain);
    }

    QString NotificationMessage::doNotShowAgainIdentifier() const {
        Q_D(const NotificationMessage);
        return d->doNotShowAgainIdentifier;
    }

    void NotificationMessage::setDoNotShowAgainIdentifier(const QString &doNotShowAgainIdentifier) {
        Q_D(NotificationMessage);
        if (d->doNotShowAgainIdentifier == doNotShowAgainIdentifier) {
            return;
        }
        d->doNotShowAgainIdentifier = doNotShowAgainIdentifier;
        Q_EMIT doNotShowAgainIdentifierChanged(doNotShowAgainIdentifier);
    }

    int NotificationMessage::textFormat() const {
        Q_D(const NotificationMessage);
        return d->textFormat;
    }

    void NotificationMessage::setTextFormat(int textFormat) {
        Q_D(NotificationMessage);
        if (d->textFormat == textFormat) {
            return;
        }
        d->textFormat = textFormat;
        Q_EMIT textFormatChanged(textFormat);
    }

    void NotificationMessage::hide() {
        Q_EMIT hidden();
    }

    void NotificationMessage::close() {
        Q_EMIT closed();
    }

}

#include "moc_NotificationMessage.cpp"
