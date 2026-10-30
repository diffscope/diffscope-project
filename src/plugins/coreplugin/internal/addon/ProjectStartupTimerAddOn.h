// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTSTARTUPTIMERAODON_H
#define DIFFSCOPE_COREPLUGIN_PROJECTSTARTUPTIMERAODON_H

#include <CoreApi/windowinterface.h>

namespace Core {
    class NotificationMessage;
}

namespace Core::Internal {

    class ProjectStartupTimerAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit ProjectStartupTimerAddOn(QObject *parent = nullptr);
        ~ProjectStartupTimerAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;

        static void startTimer();
        static void startTimerIfNotStarted();
        static qint64 stopTimerAndGetElapsedTime();

    private:
        NotificationMessage *m_initializingMessage{};
        NotificationMessage *m_finishedMessage{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTSTARTUPTIMERAODON_H
