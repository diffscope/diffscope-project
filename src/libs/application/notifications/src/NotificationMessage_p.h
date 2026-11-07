// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONMESSAGE_P_H
#define DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONMESSAGE_P_H

#include <notifications/NotificationMessage.h>

namespace Notifications {

    class NotificationMessagePrivate {
        Q_DECLARE_PUBLIC(NotificationMessage)
    public:
        NotificationMessage *q_ptr{};
        QString title{};
        QString text{};
        SVS::SVSCraft::MessageBoxIcon icon{SVS::SVSCraft::NoIcon};
        QStringList buttons{};
        int primaryButton{0};
        bool closable{true};
        bool hasProgress{false};
        double progress{0};
        bool progressAbortable{false};
        bool allowDoNotShowAgain{false};
        QString doNotShowAgainIdentifier{};
        int textFormat{Qt::AutoText};
    };

}

#endif // DIFFSCOPE_NOTIFICATIONS_NOTIFICATIONMESSAGE_P_H
