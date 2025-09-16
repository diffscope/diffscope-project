#include "projectstartuptimeraddon.h"

#include <QDateTime>
#include <QQuickView>
#include <QQuickWindow>
#include <QTimer>
#include <QSettings>
#include <QThread>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/coreinterface.h>
#include <coreplugin/projectwindowinterface.h>
#include <coreplugin/notificationmessage.h>
#include <coreplugin/internal/coreachievementsmodel.h>

namespace Core::Internal {
    ProjectStartupTimerAddOn::ProjectStartupTimerAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
        connect(CoreInterface::instance(), &CoreInterface::resetAllDoNotShowAgainRequested, this, [=] {
            setNotificationVisible(true);
            if (m_finishedMessage) {
                m_finishedMessage->setAllowDoNotShowAgain(true);
            }
        });
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
        auto window = qobject_cast<QQuickWindow *>(windowInterface->window());
        Q_ASSERT(window);
        connect(window, &QQuickWindow::sceneGraphInitialized, this, [=] {
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
                m_finishedMessage->setAllowDoNotShowAgain(notificationVisible());
                connect(m_finishedMessage, &NotificationMessage::doNotShowAgainRequested, this, [=] {
                    setNotificationVisible(false);
                    m_finishedMessage->setAllowDoNotShowAgain(false);
                });
                windowInterface->sendNotification(m_finishedMessage, notificationVisible() ? ProjectWindowInterface::AutoHide : ProjectWindowInterface::DoNotShowBubble);
                m_initializingMessage->close();
                if (elapsedTime > 60000) {
                    CoreAchievementsModel::triggerAchievementCompleted(CoreAchievementsModel::Achievement_KeepPatient);
                }
            });
        });
    }
    
    bool ProjectStartupTimerAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    static qint64 m_msecsSinceEpoch = -1;

    void ProjectStartupTimerAddOn::startTimer() {
        m_msecsSinceEpoch = QDateTime::currentMSecsSinceEpoch();
    }
    qint64 ProjectStartupTimerAddOn::stopTimerAndGetElapsedTime() {
        if (m_msecsSinceEpoch == -1) {
            return -1;
        }
        auto ret = QDateTime::currentMSecsSinceEpoch() - m_msecsSinceEpoch;
        m_msecsSinceEpoch = -1;
        return ret;
    }

    void ProjectStartupTimerAddOn::setNotificationVisible(bool visible) {
        auto settings = PluginDatabase::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue("visible", visible);
        settings->endGroup();
    }

    bool ProjectStartupTimerAddOn::notificationVisible() {
        auto settings = PluginDatabase::settings();
        settings->beginGroup(staticMetaObject.className());
        bool visible = settings->value("visible", true).toBool();
        settings->endGroup();
        return visible;
    }
}
