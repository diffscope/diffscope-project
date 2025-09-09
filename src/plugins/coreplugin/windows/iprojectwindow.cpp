#include "iprojectwindow.h"

#include <QQmlComponent>
#include <QAbstractItemModel>
#include <QJSValue>
#include <QQmlEngine>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/notificationmessage.h>
#include <coreplugin/internal/notificationmanager.h>
#include <coreplugin/quickpick.h>
#include <coreplugin/internal/actionhelper.h>
#include <coreplugin/projecttimeline.h>
#include <coreplugin/quickinput.h>
#include <coreplugin/editactionshandlerregistry.h>

namespace Core {

    static IProjectWindow *m_instance = nullptr;

    class IProjectWindowPrivate {
        Q_DECLARE_PUBLIC(IProjectWindow)
    public:
        IProjectWindow *q_ptr;
        Internal::NotificationManager *notificationManager;
        ProjectTimeline *projectTimeline;
        EditActionsHandlerRegistry *mainEditActionsHandlerRegistry;
        void init() {
            Q_Q(IProjectWindow);
            initActionContext();
            notificationManager = new Internal::NotificationManager(q);
            projectTimeline = new ProjectTimeline(q);
            mainEditActionsHandlerRegistry = new EditActionsHandlerRegistry(q);
        }

        void initActionContext() {
            Q_Q(IProjectWindow);
            auto actionContext = q->actionContext();
            QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "ProjectActions");
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

    IProjectWindow *IProjectWindow::instance() {
        return m_instance;
    }
    ProjectTimeline *IProjectWindow::projectTimeline() const {
        Q_D(const IProjectWindow);
        return d->projectTimeline;
    }

    EditActionsHandlerRegistry * IProjectWindow::mainEditActionsHandlerRegistry() const {
        Q_D(const IProjectWindow);
        return d->mainEditActionsHandlerRegistry;
    }

    void IProjectWindow::sendNotification(NotificationMessage *message, NotificationBubbleMode mode) {
        Q_D(IProjectWindow);
        d->notificationManager->addMessage(message, mode);
    }
    void IProjectWindow::sendNotification(SVS::SVSCraft::MessageBoxIcon icon, const QString &title,
                                          const QString &text, NotificationBubbleMode mode) {
        auto message = new NotificationMessage(this);
        message->setIcon(icon);
        message->setTitle(title);
        message->setText(text);
        connect(message, &NotificationMessage::closed, message, &QObject::deleteLater);
        sendNotification(message, mode);
    }
    QWindow *IProjectWindow::createWindow(QObject *parent) const {
        Q_D(const IProjectWindow);
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "ProjectWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QWindow *>(component.createWithInitialProperties({
            {"windowHandle", QVariant::fromValue(this)}
        }));
        Q_ASSERT(win);
        return win;
    }
    IProjectWindow::IProjectWindow(QObject *parent) : IProjectWindow(*new IProjectWindowPrivate, parent) {
        m_instance = this;
    }
    IProjectWindow::IProjectWindow(IProjectWindowPrivate &d, QObject *parent) : IActionWindowBase(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }
    IProjectWindow::~IProjectWindow() {
        m_instance = nullptr;
    }
    IProjectWindowRegistry *IProjectWindowRegistry::instance() {
        static IProjectWindowRegistry reg;
        return &reg;
    }
}

#include "moc_iprojectwindow.cpp"
