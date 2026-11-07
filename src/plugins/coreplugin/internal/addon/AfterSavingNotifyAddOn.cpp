// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "AfterSavingNotifyAddOn.h"

#include <CoreApi/filelocker.h>

#include <notifications/NotificationMessage.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <dspxproject/ProjectDocumentContext.h>

namespace Core::Internal {

    AfterSavingNotifyAddOn::AfterSavingNotifyAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    AfterSavingNotifyAddOn::~AfterSavingNotifyAddOn() = default;

    void AfterSavingNotifyAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        m_message = new Notifications::NotificationMessage(windowInterface->window());
        m_message->setIcon(SVS::SVSCraft::Information);
        m_message->setAllowDoNotShowAgain(false);
        connect(windowInterface->projectDocumentContext(), &DspxProject::ProjectDocumentContext::saved, this, [=, this] {
            if (!windowInterface->projectDocumentContext()->fileLocker()) {
                m_message->close();
                return;
            }
            auto path = windowInterface->projectDocumentContext()->fileLocker()->path();
            if (path.isEmpty()) {
                m_message->close();
                return;
            }
            m_message->setTitle(tr("The file has been saved to %1").arg(path));
            m_message->setAllowDoNotShowAgain(true);
            m_message->setDoNotShowAgainIdentifier("org.diffscope.core.aftersavingnotifyaddon.message");
            windowInterface->sendNotification(m_message, ProjectWindowInterface::AutoHide);
        });
    }

    void AfterSavingNotifyAddOn::extensionsInitialized() {
    }

    bool AfterSavingNotifyAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

}
