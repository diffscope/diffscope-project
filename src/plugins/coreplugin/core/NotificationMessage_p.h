#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_P_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_P_H

#include <QPointer>

#include <uishell/BubbleNotificationHandle.h>

#include <coreplugin/NotificationMessage.h>

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
