// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_P_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_P_H

#include <coreplugin/NotificationMessage.h>

#include <QPointer>

#include <uishell/BubbleNotificationHandle.h>

namespace UIShell {
    class BubbleNotificationHandle;
}

namespace Core {

    class NotificationMessagePrivate {
        Q_DECLARE_PUBLIC(NotificationMessage)
    public:
        NotificationMessage *q_ptr;
        QPointer<UIShell::BubbleNotificationHandle> handle;
        QString doNotShowAgainIdentifier;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_P_H
