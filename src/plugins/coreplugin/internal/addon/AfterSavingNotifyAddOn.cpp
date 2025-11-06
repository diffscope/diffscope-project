#include "AfterSavingNotifyAddOn.h"

#include <CoreApi/filelocker.h>

#include <coreplugin/NotificationMessage.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/ProjectDocumentContext.h>

namespace Core::Internal {

    AfterSavingNotifyAddOn::AfterSavingNotifyAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    AfterSavingNotifyAddOn::~AfterSavingNotifyAddOn() = default;

    void AfterSavingNotifyAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        m_message = new NotificationMessage(windowInterface->window());
        m_message->setIcon(SVS::SVSCraft::Information);
        m_message->setAllowDoNotShowAgain(false);
        connect(windowInterface->projectDocumentContext(), &ProjectDocumentContext::saved, this, [=, this] {
            m_message->close();
            if (!windowInterface->projectDocumentContext()->fileLocker())
                return;
            auto path = windowInterface->projectDocumentContext()->fileLocker()->path();
            if (path.isEmpty())
                return;
            m_message->setTitle(tr("The file has been saved to %1").arg(path));
            windowInterface->sendNotification(m_message, ProjectWindowInterface::DoNotShowBubble);
        });
    }

    void AfterSavingNotifyAddOn::extensionsInitialized() {
    }

    bool AfterSavingNotifyAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

}