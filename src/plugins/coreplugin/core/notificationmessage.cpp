#include "notificationmessage.h"
#include "notificationmessage_p.h"

namespace Core {
    NotificationMessage::NotificationMessage(QObject *parent) : QObject(parent), d_ptr(new NotificationMessagePrivate) {
        Q_D(NotificationMessage);
        d->q_ptr = this;
        d->handle = new UIShell::BubbleNotificationHandle;
        d->handle->setClosable(true);
        connect(d->handle, &UIShell::BubbleNotificationHandle::titleChanged, this, [=] {
            emit titleChanged(d->handle->title());
        });
        connect(d->handle, &UIShell::BubbleNotificationHandle::textChanged, this, [=] {
            emit textChanged(d->handle->text());
        });
        connect(d->handle, &UIShell::BubbleNotificationHandle::iconChanged, this, [=] {
            emit iconChanged(d->handle->icon());
        });
        connect(d->handle, &UIShell::BubbleNotificationHandle::buttonsChanged, this, [=] {
            emit buttonsChanged(d->handle->buttons());
        });
        connect(d->handle, &UIShell::BubbleNotificationHandle::primaryButtonChanged, this, [=] {
            emit primaryButtonChanged(d->handle->primaryButton());
        });
        connect(d->handle, &UIShell::BubbleNotificationHandle::closableChanged, this, [=] {
            emit closableChanged(d->handle->closable());
        });
        connect(d->handle, &UIShell::BubbleNotificationHandle::hasProgressChanged, this, [=] {
            emit hasProgressChanged(d->handle->hasProgress());
        });
        connect(d->handle, &UIShell::BubbleNotificationHandle::progressChanged, this, [=] {
            emit progressChanged(d->handle->progress());
        });
        connect(d->handle, &UIShell::BubbleNotificationHandle::progressAbortableChanged, this, [=] {
            emit progressAbortableChanged(d->handle->progressAbortable());
        });
        connect(d->handle, &UIShell::BubbleNotificationHandle::permanentlyHideableChanged, this, [=] {
            emit allowDoNotShowAgainChanged(d->handle->permanentlyHideable());
        });
        connect(d->handle, &UIShell::BubbleNotificationHandle::textFormatChanged, this, [=] {
            emit textFormatChanged(d->handle->textFormat());
        });
        connect(d->handle, &UIShell::BubbleNotificationHandle::hideClicked, this, &NotificationMessage::hidden);
        connect(d->handle, &UIShell::BubbleNotificationHandle::closeClicked, this, &NotificationMessage::closed);
        connect(d->handle, &UIShell::BubbleNotificationHandle::abortClicked, this, &NotificationMessage::progressAborted);
        connect(d->handle, &UIShell::BubbleNotificationHandle::permanentlyHideClicked, this, &NotificationMessage::doNotShowAgainRequested);
        connect(d->handle, &UIShell::BubbleNotificationHandle::buttonClicked, this, &NotificationMessage::buttonClicked);
        connect(d->handle, &UIShell::BubbleNotificationHandle::linkActivated, this, &NotificationMessage::linkActivated);
    }
    NotificationMessage::~NotificationMessage() {
        Q_D(NotificationMessage);
        if (d->handle) {
            d->handle->deleteLater();
        }
    }
    QString NotificationMessage::title() const {
        Q_D(const NotificationMessage);
        return d->handle->title();
    }
    void NotificationMessage::setTitle(const QString &title) {
        Q_D(NotificationMessage);
        d->handle->setTitle(title);
    }
    QString NotificationMessage::text() const {
        Q_D(const NotificationMessage);
        return d->handle->text();
    }
    void NotificationMessage::setText(const QString &text) {
        Q_D(NotificationMessage);
        d->handle->setText(text);
    }
    SVS::SVSCraft::MessageBoxIcon NotificationMessage::icon() const {
        Q_D(const NotificationMessage);
        return d->handle->icon();
    }
    void NotificationMessage::setIcon(SVS::SVSCraft::MessageBoxIcon icon) {
        Q_D(NotificationMessage);
        d->handle->setIcon(icon);
    }
    QStringList NotificationMessage::buttons() const {
        Q_D(const NotificationMessage);
        return d->handle->buttons();
    }
    void NotificationMessage::setButtons(const QStringList &buttons) {
        Q_D(NotificationMessage);
        d->handle->setButtons(buttons);
    }
    int NotificationMessage::primaryButton() const {
        Q_D(const NotificationMessage);
        return d->handle->primaryButton();
    }
    void NotificationMessage::setPrimaryButton(int primaryButton) {
        Q_D(NotificationMessage);
        d->handle->setPrimaryButton(primaryButton);
    }
    bool NotificationMessage::closable() const {
        Q_D(const NotificationMessage);
        return d->handle->closable();
    }
    void NotificationMessage::setClosable(bool closable) {
        Q_D(NotificationMessage);
        d->handle->setClosable(closable);
    }
    bool NotificationMessage::hasProgress() const {
        Q_D(const NotificationMessage);
        return d->handle->hasProgress();
    }
    void NotificationMessage::setHasProgress(bool hasProgress) {
        Q_D(NotificationMessage);
        d->handle->setHasProgress(hasProgress);
    }
    double NotificationMessage::progress() const {
        Q_D(const NotificationMessage);
        return d->handle->progress();
    }
    void NotificationMessage::setProgress(double progress) {
        Q_D(NotificationMessage);
        d->handle->setProgress(progress);
    }
    bool NotificationMessage::progressAbortable() const {
        Q_D(const NotificationMessage);
        return d->handle->progressAbortable();
    }
    void NotificationMessage::setProgressAbortable(bool progressAbortable) {
        Q_D(NotificationMessage);
        d->handle->setProgressAbortable(progressAbortable);
    }
    bool NotificationMessage::allowDoNotShowAgain() const {
        Q_D(const NotificationMessage);
        return d->handle->permanentlyHideable();
    }
    void NotificationMessage::setAllowDoNotShowAgain(bool allow) {
        Q_D(NotificationMessage);
        d->handle->setPermanentlyHideable(allow);
    }
    int NotificationMessage::textFormat() const {
        Q_D(const NotificationMessage);
        return d->handle->textFormat();
    }
    void NotificationMessage::setTextFormat(int format) {
        Q_D(NotificationMessage);
        d->handle->setTextFormat(format);
    }
    void NotificationMessage::hide() {
        Q_D(NotificationMessage);
        emit d->handle->hideClicked();
    }
    void NotificationMessage::close() {
        Q_D(NotificationMessage);
        emit d->handle->closeClicked();
    }
}

#include "moc_notificationmessage.cpp"