#include "ProjectWindowInterface.h"

#include <QQmlComponent>
#include <QAbstractItemModel>
#include <QJSValue>
#include <QQmlEngine>
#include <QQuickWindow>

#include <SVSCraftQuick/StatusTextContext.h>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/coreinterface.h>
#include <coreplugin/notificationmessage.h>
#include <coreplugin/internal/notificationmanager.h>
#include <coreplugin/quickpick.h>
#include <coreplugin/internal/actionhelper.h>
#include <coreplugin/projecttimeline.h>
#include <coreplugin/quickinput.h>
#include <coreplugin/editactionshandlerregistry.h>

namespace Core {

    static ProjectWindowInterface *m_instance = nullptr;

    class ProjectWindowInterfacePrivate {
        Q_DECLARE_PUBLIC(ProjectWindowInterface)
    public:
        ProjectWindowInterface *q_ptr;
        Internal::NotificationManager *notificationManager;
        ProjectTimeline *projectTimeline;
        EditActionsHandlerRegistry *mainEditActionsHandlerRegistry;
        void init() {
            Q_Q(ProjectWindowInterface);
            initActionContext();
            notificationManager = new Internal::NotificationManager(q);
            projectTimeline = new ProjectTimeline(q);
            mainEditActionsHandlerRegistry = new EditActionsHandlerRegistry(q);
        }

        void initActionContext() {
            Q_Q(ProjectWindowInterface);
            auto actionContext = q->actionContext();
            QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.Core", "ProjectActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"windowHandle", QVariant::fromValue(q)},
            });
            o->setParent(q);
            QMetaObject::invokeMethod(o, "registerToContext", actionContext);
        }
    };

    ProjectWindowInterface *ProjectWindowInterface::instance() {
        return m_instance;
    }
    ProjectTimeline *ProjectWindowInterface::projectTimeline() const {
        Q_D(const ProjectWindowInterface);
        return d->projectTimeline;
    }

    EditActionsHandlerRegistry * ProjectWindowInterface::mainEditActionsHandlerRegistry() const {
        Q_D(const ProjectWindowInterface);
        return d->mainEditActionsHandlerRegistry;
    }

    void ProjectWindowInterface::sendNotification(NotificationMessage *message, NotificationBubbleMode mode) {
        Q_D(ProjectWindowInterface);
        d->notificationManager->addMessage(message, mode);
    }
    void ProjectWindowInterface::sendNotification(SVS::SVSCraft::MessageBoxIcon icon, const QString &title,
                                          const QString &text, NotificationBubbleMode mode) {
        auto message = new NotificationMessage(this);
        message->setIcon(icon);
        message->setTitle(title);
        message->setText(text);
        connect(message, &NotificationMessage::closed, message, &QObject::deleteLater);
        sendNotification(message, mode);
    }
    QWindow *ProjectWindowInterface::createWindow(QObject *parent) const {
        Q_D(const ProjectWindowInterface);
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.Core", "ProjectWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QQuickWindow *>(component.createWithInitialProperties({
            {"windowHandle", QVariant::fromValue(this)}
        }));
        Q_ASSERT(win);
        SVS::StatusTextContext::setStatusContext(win, new SVS::StatusTextContext(win));
        SVS::StatusTextContext::setContextHelpContext(win, new SVS::StatusTextContext(win));
        return win;
    }
    ProjectWindowInterface::ProjectWindowInterface(QObject *parent) : ProjectWindowInterface(*new ProjectWindowInterfacePrivate, parent) {
        m_instance = this;
    }
    ProjectWindowInterface::ProjectWindowInterface(ProjectWindowInterfacePrivate &d, QObject *parent) : ActionWindowInterfaceBase(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }
    ProjectWindowInterface::~ProjectWindowInterface() {
        m_instance = nullptr;
    }
    ProjectWindowInterfaceRegistry *ProjectWindowInterfaceRegistry::instance() {
        static ProjectWindowInterfaceRegistry reg;
        return &reg;
    }
}

#include "moc_projectwindowinterface.cpp"
