#ifndef DIFFSCOPE_COREPLUGIN_PROJECTSTARTUPTIMERAODON_H
#define DIFFSCOPE_COREPLUGIN_PROJECTSTARTUPTIMERAODON_H

#include <CoreApi/iwindow.h>

namespace Core {
    class NotificationMessage;
}

namespace Core::Internal {

    class ProjectStartupTimerAddOn : public IWindowAddOn {
        Q_OBJECT
    public:
        explicit ProjectStartupTimerAddOn(QObject *parent = nullptr);
        ~ProjectStartupTimerAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static void startTimer();
        static qint64 stopTimerAndGetElapsedTime();

    private:
        NotificationMessage *m_initializingMessage{};
        NotificationMessage *m_finishedMessage{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTSTARTUPTIMERAODON_H
