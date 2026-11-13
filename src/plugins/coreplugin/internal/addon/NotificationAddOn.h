// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONADDON_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONADDON_H

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class NotificationViewModel;

    class NotificationAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(NotificationViewModel *notificationViewModel READ notificationViewModel CONSTANT)
    public:
        explicit NotificationAddOn(QObject *parent = nullptr);
        ~NotificationAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;

        NotificationViewModel *notificationViewModel() const;

    Q_SIGNALS:
        void _diffscope_statusTipTriggered();
        void showPanelRequested();

    private:
        NotificationViewModel *m_notificationViewModel{};

    };

}

#endif //DIFFSCOPE_COREPLUGIN_NOTIFICATIONADDON_H
