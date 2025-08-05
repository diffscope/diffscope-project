#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONADDON_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONADDON_H

#include <CoreApi/iwindow.h>

namespace Core::Internal {

    class NotificationManager;

    class NotificationAddOn : public IWindowAddOn {
        Q_OBJECT
        Q_PROPERTY(NotificationManager *notificationManager READ notificationManager CONSTANT)
    public:
        explicit NotificationAddOn(QObject *parent = nullptr);
        ~NotificationAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        NotificationManager *notificationManager() const;

    private:
        NotificationManager *m_notificationManager{};

    };

}

#endif //DIFFSCOPE_COREPLUGIN_NOTIFICATIONADDON_H
