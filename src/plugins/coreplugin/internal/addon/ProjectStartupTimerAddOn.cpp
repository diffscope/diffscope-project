#include "ProjectStartupTimerAddOn.h"

#include <QDateTime>
#include <QLoggingCategory>
#include <QQuickView>
#include <QQuickWindow>
#include <QSettings>
#include <QThread>
#include <QTimer>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/NotificationMessage.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcProjectStartupTimerAddOn, "diffscope.core.projectstartuptimeraddon")

    ProjectStartupTimerAddOn::ProjectStartupTimerAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ProjectStartupTimerAddOn::~ProjectStartupTimerAddOn() = default;

    void ProjectStartupTimerAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        m_initializingMessage = new NotificationMessage(windowInterface->window());
        m_initializingMessage->setIcon(SVS::SVSCraft::Information);
        m_initializingMessage->setTitle(tr("Initializing project window..."));
        m_initializingMessage->setClosable(false);
        windowInterface->sendNotification(m_initializingMessage);
    }

    void ProjectStartupTimerAddOn::extensionsInitialized() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        auto window = windowInterface->window();
        window->installEventFilter(this);
    }

    bool ProjectStartupTimerAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    static qint64 m_msecsSinceEpoch = -1;

    void ProjectStartupTimerAddOn::startTimer() {
        m_msecsSinceEpoch = QDateTime::currentMSecsSinceEpoch();
        qCInfo(lcProjectStartupTimerAddOn) << "Project startup timer started at" << m_msecsSinceEpoch;
    }
    qint64 ProjectStartupTimerAddOn::stopTimerAndGetElapsedTime() {
        if (m_msecsSinceEpoch == -1) {
            qCWarning(lcProjectStartupTimerAddOn) << "Project startup timer not started";
            return -1;
        }
        auto ret = QDateTime::currentMSecsSinceEpoch() - m_msecsSinceEpoch;
        qCInfo(lcProjectStartupTimerAddOn) << "Project startup timer stopped in" << ret << "ms";
        m_msecsSinceEpoch = -1;
        return ret;
    }

    bool ProjectStartupTimerAddOn::eventFilter(QObject *watched, QEvent *event) {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        auto window = windowInterface->window();
        if (event->type() == QEvent::Expose && window->isExposed()) {
            QTimer::singleShot(0, [=] {
                m_finishedMessage = new NotificationMessage(windowInterface->window());
                m_finishedMessage->setIcon(SVS::SVSCraft::Success);
                auto elapsedTime = stopTimerAndGetElapsedTime();
                if (elapsedTime != -1) {
                    double second = elapsedTime / 1000.0;
                    m_finishedMessage->setTitle(tr("Project window initialized in %1 seconds").arg(second, 0, 'f', 3));
                } else {
                    m_finishedMessage->setTitle(tr("Project window initialized"));
                }
                m_finishedMessage->setAllowDoNotShowAgain(true);
                m_finishedMessage->setDoNotShowAgainIdentifier("org.diffscope.core.projectstartuptimeraddon.finishedmessage");
                windowInterface->sendNotification(m_finishedMessage, ProjectWindowInterface::AutoHide);
                m_initializingMessage->close();
            });
            window->removeEventFilter(this);
        }
        return WindowInterfaceAddOn::eventFilter(watched, event);
    }
}
