#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONADDON_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONADDON_H

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class NotificationViewModel;

    class NotificationAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(NotificationViewModel *notificationManager READ notificationManager CONSTANT)
    public:
        explicit NotificationAddOn(QObject *parent = nullptr);
        ~NotificationAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        NotificationViewModel *notificationManager() const;

    signals:
        void _diffscope_statusTipTriggered();
        void deactivateIndicator();
        void showPanelRequested();

    private:
        NotificationViewModel *m_notificationViewModel{};

    };

}

#endif //DIFFSCOPE_COREPLUGIN_NOTIFICATIONADDON_H
