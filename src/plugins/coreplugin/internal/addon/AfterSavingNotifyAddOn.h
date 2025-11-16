#ifndef DIFFSCOPE_COREPLUGIN_AFTERSAVINGNOTIFYADDON_H
#define DIFFSCOPE_COREPLUGIN_AFTERSAVINGNOTIFYADDON_H

#include <CoreApi/windowinterface.h>

namespace Core {
    class NotificationMessage;
}

namespace Core::Internal {

    class AfterSavingNotifyAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit AfterSavingNotifyAddOn(QObject *parent = nullptr);
        ~AfterSavingNotifyAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
        NotificationMessage *m_message{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_AFTERSAVINGNOTIFYADDON_H